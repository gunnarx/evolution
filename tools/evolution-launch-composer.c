/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * evolution-sendmail.c -
 * Copyright (C) 2002, Ximian, Inc.
 *
 * Authors:
 *   Christopher James Lahey <clahey@ximian.com>
 *   Federico Mena Quintero <federico@ximian.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file; if not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 **/

#include <config.h>

#include <stdlib.h>
#include <Evolution-Composer.h>
#include <bonobo/bonobo-object-client.h>
#include <popt.h>
#include <gal/util/e-util.h>
#include <gal/util/e-i18n.h>
#include <liboaf/liboaf.h>
#include <gtk/gtkmain.h>

#include <bonobo/bonobo-main.h>

#include <libgnomeui/gnome-init.h>

#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-xfer.h>
#include <libgnomevfs/gnome-vfs-init.h>

#define READ_CHUNK_SIZE 8192

#define E_SHELL_OAFIID  "OAFIID:GNOME_Evolution_Shell"
#define COMPOSER_OAFIID "OAFIID:GNOME_Evolution_Mail_Composer"
#define NOTNULL(x) ((x) ? (x) : "")

typedef struct {
	char *filename;
	char *basename;
	char *content;
	int   size;
	char *description;
	char *content_type;
	gboolean show_inline;
} attachment_t;

static GList *attachments = NULL; /* Of type attachment_t */

/* Lists of char */
static GList *to_recipients = NULL;
static GList *cc_recipients = NULL;
static GList *bcc_recipients = NULL;

char *subject;
char *body;


static attachment_t *
attachment_new (void)
{
	attachment_t *a;

	a = g_new (attachment_t, 1);
	a->filename = NULL;
	a->basename = NULL;
	a->content = NULL;
	a->size = 0;
	a->description = NULL;
	a->content_type = NULL;
	a->show_inline = FALSE;

	return a;
}

static void
attachment_free (attachment_t *attachment)
{
	g_free (attachment->filename);
	g_free (attachment->basename);
	g_free (attachment->content);
	g_free (attachment->description);
	g_free (attachment->content_type);
	g_free (attachment);
}

GnomeVFSResult
elc_read_entire_file (const char *uri,
		      int *file_size,
		      char **file_contents,
		      char **content_type)
{
	GnomeVFSResult result;
	GnomeVFSHandle *handle;
	char *buffer;
	GnomeVFSFileSize total_bytes_read;
	GnomeVFSFileSize bytes_read;

	*file_size = 0;
	*file_contents = NULL;

	/* Open the file. */
	result = gnome_vfs_open (&handle, uri, GNOME_VFS_OPEN_READ);
	if (result != GNOME_VFS_OK) {
		return result;
	}

	/* Read the whole thing. */
	buffer = NULL;
	total_bytes_read = 0;
	do {
		buffer = g_realloc (buffer, total_bytes_read + READ_CHUNK_SIZE);
		result = gnome_vfs_read (handle,
					 buffer + total_bytes_read,
					 READ_CHUNK_SIZE,
					 &bytes_read);
		if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_EOF) {
			g_free (buffer);
			gnome_vfs_close (handle);
			return result;
		}

		/* Check for overflow. */
		if (total_bytes_read + bytes_read < total_bytes_read) {
			g_free (buffer);
			gnome_vfs_close (handle);
			return GNOME_VFS_ERROR_TOO_BIG;
		}

		total_bytes_read += bytes_read;
	} while (result == GNOME_VFS_OK);

	if (content_type) {
		GnomeVFSFileInfo *info;

		info = gnome_vfs_file_info_new ();
		result = gnome_vfs_get_file_info_from_handle (handle, info,
							      GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
							      GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
		if (result == GNOME_VFS_OK)
			*content_type = g_strdup (gnome_vfs_file_info_get_mime_type (info));
		else
			*content_type = g_strdup ("application/octet-stream");

		gnome_vfs_file_info_unref (info);
	}

	/* Close the file. */
	result = gnome_vfs_close (handle);
	if (result != GNOME_VFS_OK) {
		g_free (buffer);
		return result;
	}

	/* Return the file. */
	*file_size = total_bytes_read;
	*file_contents = g_realloc (buffer, total_bytes_read);
	return GNOME_VFS_OK;
}

GNOME_Evolution_Composer_RecipientList *
make_recipient_list (GList *addresses)
{
	GNOME_Evolution_Composer_RecipientList *recipients;
	int i, n;

	n = g_list_length (addresses);

	recipients = GNOME_Evolution_Composer_RecipientList__alloc ();
	recipients->_release = TRUE;
	recipients->_maximum = recipients->_length = n;
	recipients->_buffer = CORBA_sequence_GNOME_Evolution_Composer_Recipient_allocbuf (n);

	for (i = 0; i < n; i++) {
		g_assert (addresses != NULL && addresses->data != NULL);

		recipients->_buffer[i].name = CORBA_string_dup ("");
		recipients->_buffer[i].address = CORBA_string_dup (addresses->data);

		addresses = addresses->next;
	}

	return recipients;
}

static void
do_send (BonoboObjectClient *bonobo_server)
{
	GNOME_Evolution_Composer composer_server;
	CORBA_Environment ev;

	GNOME_Evolution_Composer_AttachmentData *attach_data;

	GNOME_Evolution_Composer_RecipientList *to_list, *cc_list, *bcc_list;

	composer_server = bonobo_object_corba_objref (BONOBO_OBJECT (bonobo_server));

	CORBA_exception_init (&ev);

	while (attachments) {
		attachment_t *attachment = attachments->data;
		GList *temp;

		attach_data = GNOME_Evolution_Composer_AttachmentData__alloc();
		attach_data->_maximum = attach_data->_length = attachment->size;
		attach_data->_buffer = CORBA_sequence_CORBA_char_allocbuf (attach_data->_length);
		memcpy (attach_data->_buffer, attachment->content, attachment->size);

		GNOME_Evolution_Composer_attachData (composer_server,
						     NOTNULL (attachment->content_type),
						     NOTNULL (attachment->basename),
						     NOTNULL (attachment->description),
						     attachment->show_inline,
						     attach_data,
						     &ev);

		if (ev._major != CORBA_NO_EXCEPTION) {
			g_printerr ("evolution-launch-composer.c: Exception while creating an attachment\n");
			CORBA_exception_free (&ev);
			return;
		}

		CORBA_free (attach_data);

		attachment_free (attachment);

		temp = attachments;
		attachments = g_list_remove_link (attachments, attachments);
		g_list_free_1 (temp);
	}

	to_list = make_recipient_list (to_recipients);
	cc_list = make_recipient_list (cc_recipients);
	bcc_list = make_recipient_list (bcc_recipients);

	GNOME_Evolution_Composer_setHeaders (composer_server, "", to_list, cc_list, bcc_list, NOTNULL (subject), &ev);

	CORBA_free (to_list);
	CORBA_free (cc_list);
	CORBA_free (bcc_list);

	GNOME_Evolution_Composer_show (composer_server, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_printerr ("evolution-sendmail.c: I couldn't show the composer via CORBA! Aagh.\n");
		CORBA_exception_free (&ev);
		return;
	}

	CORBA_exception_free (&ev);
}


static BonoboObjectClient *
get_composer ()
{
	BonoboObjectClient *bonobo_server;

	/* First, I obtain an object reference that represents the Shell, to make sure it's running. */
	bonobo_server = bonobo_object_activate (E_SHELL_OAFIID, 0);

	g_return_val_if_fail (bonobo_server != NULL, NULL);

	/* Next, I obtain an object reference that represents the Composer. */
	bonobo_server = bonobo_object_activate (COMPOSER_OAFIID, 0);

	return bonobo_server;
}

static gboolean
composer_timeout (gpointer data)
{
	BonoboObjectClient *bonobo_server;

	bonobo_server = get_composer ();

	if (bonobo_server) {
		do_send (bonobo_server);
		gtk_main_quit ();
		return FALSE;
	} else {
		return TRUE;
	}
}

static void
cb (poptContext con,
    enum poptCallbackReason reason,
    /*@null@*/ const struct poptOption * opt,
    /*@null@*/ const char * arg,
    /*@null@*/ const void * data)
{
	const char *name = opt->longName;

	if (!strcmp (name, "to"))
		to_recipients = g_list_prepend (to_recipients, g_strdup (arg));
	else if (!strcmp (name, "cc"))
		cc_recipients = g_list_prepend (cc_recipients, g_strdup (arg));
	else if (!strcmp (name, "bcc"))
		bcc_recipients = g_list_prepend (bcc_recipients, g_strdup (arg));
	else if (!strcmp (name, "add-attachment")) {
		attachment_t *new_attach;

		new_attach = attachment_new ();
		new_attach->filename = g_strdup (arg);
		new_attach->basename = strrchr (new_attach->filename, '/');
		if (new_attach->basename)
			new_attach->basename ++;
		else
			new_attach->basename = new_attach->filename;
		new_attach->basename = g_strdup (new_attach->basename);
		if (new_attach->filename)
			elc_read_entire_file (new_attach->filename,
					      &new_attach->size,
					      &new_attach->content,
					      &new_attach->content_type);
		new_attach->description = NULL;
		new_attach->show_inline = FALSE;
		if (new_attach->content) {
			attachments = g_list_prepend (attachments, new_attach);
		} else {
			attachment_free (new_attach);
		}
	} else if (!strcmp (name, "file-name")) {
		if (attachments) {
			attachment_t *attachment = attachments->data;
			if (attachment->basename)
				g_free (attachment->basename);

			attachment->basename = g_strdup (arg);
		} else {
			g_printerr ("You must specify the --add-attachment option before --file-name");
			exit (EXIT_FAILURE);
		}
	} else if (!strcmp (name, "content-type")) {
		if (attachments) {
			attachment_t *attachment = attachments->data;
			if (attachment->content_type)
				g_free (attachment->content_type);

			attachment->content_type = g_strdup (arg);
		} else {
			g_printerr ("You must specify the --add-attachment option before --content-type");
			exit (EXIT_FAILURE);
		}
	} else if (!strcmp (name, "description")) {
		if (attachments) {
			attachment_t *attachment = attachments->data;
			if (attachment->description)
				g_free (attachment->description);

			attachment->description = g_strdup (arg);
		} else {
			g_printerr ("You must specify the --add-attachment option before --description");
			exit (EXIT_FAILURE);
		}
	} else if (!strcmp (name, "show-inline")) {
		if (attachments) {
			attachment_t *attachment = attachments->data;
			attachment->show_inline = TRUE;
		} else {
			g_printerr ("You must specify the --add-attachment option before --show-inline");
			exit (EXIT_FAILURE);
		}
	}
}

static struct poptOption cap_options[] = {
	{ NULL, '\0', POPT_ARG_CALLBACK, cb, 0, NULL, NULL },
	{ "to", 't', POPT_ART_STRING, NULL, 0, N_("One of the \"To:\" recipients."), NULL },
	{ "cc", 0, POPT_ART_STRING, NULL, 0, N_("One of the \"CC:\" recipients."), NULL },
	{ "bcc", 0, POPT_ART_STRING, NULL, 0, N_("One of the \"BCC:\" recipients."), NULL },
	{ "subject", 's', POPT_ARG_STRING, &subject, 0, N_("Default subject for the message."), NULL },
	{ "body", 'b', POPT_ARG_STRING, &body, 0, N_("Message body text."), NULL },
	{ "add-attachment", 'a', POPT_ARG_STRING, NULL, 0, N_("An attachment to add."), NULL },
	{ "content-type", 'c', POPT_ARG_STRING, NULL, 0, N_("Content type of the attachment."), NULL },
	{ "file-name", 'f', POPT_ARG_STRING, NULL, 0, N_("The filename to display in the mail."), NULL },
	{ "description", 'd', POPT_ARG_STRING, NULL, 0, N_("Description of the attachment."), NULL },
	{ "show-inline", 'i', POPT_ARG_NONE, NULL, 0, N_("Mark attachment to be shown inline by default."), NULL },
	{ NULL, '\0', 0, NULL, 0 }
};

int
main(int argc, char *argv[])
{
	BonoboObjectClient *bonobo_server;

        bindtextdomain (PACKAGE, EVOLUTION_LOCALEDIR);
        textdomain (PACKAGE);

	gnome_vfs_init ();

	gnome_init_with_popt_table ("evolution-launch-composer", VERSION, argc, argv, cap_options, 0, NULL);

	if (bonobo_init (CORBA_OBJECT_NIL, CORBA_OBJECT_NIL, CORBA_OBJECT_NIL) == FALSE)
		g_error (_("Could not initialize Bonobo"));

	bonobo_server = get_composer ();

	to_recipients = g_list_reverse (to_recipients);
	cc_recipients = g_list_reverse (cc_recipients);
	bcc_recipients = g_list_reverse (bcc_recipients);
	attachments = g_list_reverse (attachments);

	if (bonobo_server) {
		do_send (bonobo_server);
	} else {
		g_timeout_add(1000, composer_timeout, NULL);
		gtk_main ();
	}

	return 0;
}
