/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * e-contact-editor.c
 * Copyright (C) 2000  Helix Code, Inc.
 * Author: Chris Lahey <clahey@helixcode.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gnome.h>
#include "e-contact-editor.h"
static void e_contact_editor_init		(EContactEditor		 *card);
static void e_contact_editor_class_init	(EContactEditorClass	 *klass);
static void e_contact_editor_set_arg (GtkObject *o, GtkArg *arg, guint arg_id);
static void e_contact_editor_get_arg (GtkObject *object, GtkArg *arg, guint arg_id);
static void e_contact_editor_destroy (GtkObject *object);

static GtkWidget *e_contact_editor_build_dialog(EContactEditor *editor, gchar *entry_id, gchar *label_id, gchar *title, GList **list, GnomeUIInfo **info);
static void _email_arrow_pressed (GtkWidget *widget, GdkEventButton *button, EContactEditor *editor);
static void _phone_arrow_pressed (GtkWidget *widget, GdkEventButton *button, EContactEditor *editor);
static void _address_arrow_pressed (GtkWidget *widget, GdkEventButton *button, EContactEditor *editor);

static GtkVBoxClass *parent_class = NULL;

#if 0
enum {
	E_CONTACT_EDITOR_RESIZE,
	E_CONTACT_EDITOR_LAST_SIGNAL
};

static guint e_contact_editor_signals[E_CONTACT_EDITOR_LAST_SIGNAL] = { 0 };
#endif

/* The arguments we take */
enum {
	ARG_0,
	ARG_CARD
};

enum {
	DYNAMIC_LIST_EMAIL,
	DYNAMIC_LIST_PHONE,
	DYNAMIC_LIST_ADDRESS
};

GtkType
e_contact_editor_get_type (void)
{
  static GtkType contact_editor_type = 0;

  if (!contact_editor_type)
    {
      static const GtkTypeInfo contact_editor_info =
      {
        "EContactEditor",
        sizeof (EContactEditor),
        sizeof (EContactEditorClass),
        (GtkClassInitFunc) e_contact_editor_class_init,
        (GtkObjectInitFunc) e_contact_editor_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      contact_editor_type = gtk_type_unique (gtk_vbox_get_type (), &contact_editor_info);
    }

  return contact_editor_type;
}

static void
e_contact_editor_class_init (EContactEditorClass *klass)
{
  GtkObjectClass *object_class;
  GtkVBoxClass *vbox_class;

  object_class = (GtkObjectClass*) klass;
  vbox_class = (GtkVBoxClass *) klass;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

#if 0  
  e_contact_editor_signals[E_CONTACT_EDITOR_RESIZE] =
	  gtk_signal_new ("resize",
			  GTK_RUN_LAST,
			  object_class->type,
			  GTK_SIGNAL_OFFSET (EContactEditorClass, resize),
			  gtk_marshal_NONE__NONE,
			  GTK_TYPE_NONE, 0);
  
  
  gtk_object_class_add_signals (object_class, e_contact_editor_signals, E_CONTACT_EDITOR_LAST_SIGNAL);
#endif
  
  gtk_object_add_arg_type ("EContactEditor::card", GTK_TYPE_OBJECT, 
			   GTK_ARG_READWRITE, ARG_CARD);
 
  object_class->set_arg = e_contact_editor_set_arg;
  object_class->get_arg = e_contact_editor_get_arg;
  object_class->destroy = e_contact_editor_destroy;
}

static void
_add_image(GtkTable *table, gchar *image, int left, int right, int top, int bottom)
{
	gtk_table_attach(table,
			 gtk_widget_new(gtk_alignment_get_type(),
					"child", gnome_pixmap_new_from_file(image),
					"xalign", (double) 0,
					"yalign", (double) 0,
					"xscale", (double) 0,
					"yscale", (double) 0,
					NULL),
			 left, right, top, bottom,
			 GTK_FILL, GTK_FILL,
			 0, 0);
}

static void
_add_images(GtkTable *table)
{
	_add_image(table, DATADIR "/evolution/head.png", 0, 1, 0, 4);
	_add_image(table, DATADIR "/evolution/phone.png", 4, 5, 0, 4);
	_add_image(table, DATADIR "/evolution/email.png", 0, 1, 5, 7);
	_add_image(table, DATADIR "/evolution/web.png", 0, 1, 8, 10);
	_add_image(table, DATADIR "/evolution/snailmail.png", 4, 5, 5, 10);
}

static void
_add_details_images(GtkTable *table)
{
	_add_image(table, DATADIR "/evolution/briefcase.png", 0, 1, 0, 2);
	_add_image(table, DATADIR "/evolution/head.png", 0, 1, 4, 6);
	_add_image(table, DATADIR "/evolution/netmeeting.png", 0, 1, 7, 9);
	_add_image(table, DATADIR "/evolution/netfreebusy.png", 0, 1, 10, 12);
}

static void
_replace_button(EContactEditor *editor, gchar *button_xml, gchar *image, GtkSignalFunc func)
{
	GladeXML *gui = editor->gui;
	GtkWidget *button = glade_xml_get_widget(gui, button_xml);
	gchar *image_temp;
	image_temp = g_strdup_printf("%s%s", DATADIR "/evolution/", image);
	gtk_container_add(GTK_CONTAINER(button),
			  gnome_pixmap_new_from_file(image_temp));
	g_free(image_temp);
	gtk_signal_connect(GTK_OBJECT(button), "button_press_event", func, editor);
			   
}

static void
_replace_buttons(EContactEditor *editor)
{
	_replace_button(editor, "button-phone1", "arrow.png", _phone_arrow_pressed);
	_replace_button(editor, "button-phone2", "arrow.png", _phone_arrow_pressed);
	_replace_button(editor, "button-phone3", "arrow.png", _phone_arrow_pressed);
	_replace_button(editor, "button-phone4", "arrow.png", _phone_arrow_pressed);
	_replace_button(editor, "button-address1", "arrow.png", _address_arrow_pressed);
	_replace_button(editor, "button-email1", "arrow.png", _email_arrow_pressed);
}

static void
e_contact_editor_init (EContactEditor *e_contact_editor)
{
	GladeXML *gui;
	GtkAdjustment *adjustment;

	/*   e_contact_editor->card = NULL;*/
	gui = glade_xml_new (EVOLUTION_GLADEDIR "/contact-editor.glade", NULL);
	e_contact_editor->gui = gui;
	gtk_widget_reparent(glade_xml_get_widget(gui, "notebook-contact-editor"),
			    GTK_WIDGET(e_contact_editor));

	_add_images(GTK_TABLE(glade_xml_get_widget(gui, "table-contact-editor-general")));
	_add_details_images(GTK_TABLE(glade_xml_get_widget(gui, "table-contact-editor-details")));
	_replace_buttons(e_contact_editor);

	gtk_object_get(GTK_OBJECT(glade_xml_get_widget(gui, "text-comments")),
		       "vadjustment", &adjustment,
		       NULL);
	gtk_range_set_adjustment(GTK_RANGE(glade_xml_get_widget(gui, "vscrollbar-comments")),
				 adjustment);
	
	e_contact_editor->email_info = NULL;
	e_contact_editor->phone_info = NULL;
	e_contact_editor->address_info = NULL;
	e_contact_editor->email_popup = NULL;
	e_contact_editor->phone_popup = NULL;
	e_contact_editor->address_popup = NULL;
	e_contact_editor->email_list = NULL;
	e_contact_editor->phone_list = NULL;
	e_contact_editor->address_list = NULL;
}

void
e_contact_editor_destroy (GtkObject *object) {
	EContactEditor *e_contact_editor = E_CONTACT_EDITOR(object);
	
	if (e_contact_editor->email_list) {
		g_list_foreach(e_contact_editor->email_list, (GFunc) g_free, NULL);
		g_list_free(e_contact_editor->email_list);
	}
	if (e_contact_editor->email_info) {
		g_free(e_contact_editor->email_info);
	}
	if (e_contact_editor->email_popup) {
		gtk_widget_unref(e_contact_editor->email_popup);
	}
	
	if (e_contact_editor->phone_list) {
		g_list_foreach(e_contact_editor->phone_list, (GFunc) g_free, NULL);
		g_list_free(e_contact_editor->phone_list);
	}
	if (e_contact_editor->phone_info) {
		g_free(e_contact_editor->phone_info);
	}
	if (e_contact_editor->phone_popup) {
		gtk_widget_unref(e_contact_editor->phone_popup);
	}
	
	if (e_contact_editor->address_list) {
		g_list_foreach(e_contact_editor->address_list, (GFunc) g_free, NULL);
		g_list_free(e_contact_editor->address_list);
	}
	if (e_contact_editor->address_info) {
		g_free(e_contact_editor->address_info);
	}
	if (e_contact_editor->address_popup) {
		gtk_widget_unref(e_contact_editor->address_popup);
	}

	gtk_object_unref(GTK_OBJECT(e_contact_editor->gui));
}

GtkWidget*
e_contact_editor_new (void *card)
{
	GtkWidget *widget = GTK_WIDGET (gtk_type_new (e_contact_editor_get_type ()));
	gtk_object_set (GTK_OBJECT(widget),
			"card", card,
			NULL);
	return widget;
}

static void
e_contact_editor_set_arg (GtkObject *o, GtkArg *arg, guint arg_id)
{
	EContactEditor *e_contact_editor;

	e_contact_editor = E_CONTACT_EDITOR (o);
	
	switch (arg_id){
	case ARG_CARD:
	  /*	  e_contact_editor->card = GTK_VALUE_POINTER (*arg);
	  _update_card(e_contact_editor);
	  gnome_canvas_item_request_update (item);*/
	  break;
	}
}

static void
e_contact_editor_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	EContactEditor *e_contact_editor;

	e_contact_editor = E_CONTACT_EDITOR (object);

	switch (arg_id) {
	case ARG_CARD:
	  /* GTK_VALUE_POINTER (*arg) = e_contact_editor->card; */
	  break;
	default:
	  arg->type = GTK_TYPE_INVALID;
	  break;
	}
}

static void
_popup_position(GtkMenu *menu,
		gint *x,
		gint *y,
		gpointer data)
{
	GtkWidget *button = GTK_WIDGET(data);
	GtkRequisition request;
	int mh, mw;
	gdk_window_get_origin (button->window, x, y);
	*x += button->allocation.width;
	*y += button->allocation.height;

	gtk_widget_size_request(GTK_WIDGET(menu), &request);

	mh = request.height;
	mw = request.width;

	*x -= mw;
	if (*x < 0)
		*x = 0;
	
	if (*y < 0)
		*y = 0;
	
	if ((*x + mw) > gdk_screen_width ())
		*x = gdk_screen_width () - mw;
	
	if ((*y + mh) > gdk_screen_height ())
		*y = gdk_screen_height () - mh;
}

static void
_arrow_pressed (GtkWidget *widget, GdkEventButton *button, EContactEditor *editor, GtkWidget *popup, GList **list, GnomeUIInfo **info, gchar *label, gchar *entry, gchar *dialog_title)
{
	gint menu_item;
	gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "button_press_event");
	gtk_widget_realize(popup);
	menu_item = gnome_popup_menu_do_popup_modal(popup, _popup_position, widget, button, editor);
	if ( menu_item != -1 ) {
		if (menu_item == g_list_length (*list)) {
			e_contact_editor_build_dialog(editor, entry, label, dialog_title, list, info);
		} else {
			gtk_object_set(GTK_OBJECT(glade_xml_get_widget(editor->gui, label)),
				       "label", g_list_nth_data(*list, menu_item),
				       NULL);
		}
	}
}

static void
e_contact_editor_build_ui_info(GList *list, GnomeUIInfo **infop)
{
	GnomeUIInfo *info;
	GnomeUIInfo singleton = { GNOME_APP_UI_ITEM, NULL, NULL, NULL, NULL, NULL, GNOME_APP_PIXMAP_NONE, 0, 0, 0, NULL };
	GnomeUIInfo end = GNOMEUIINFO_END;
	int length;
	int i;

	info = *infop;

	if ( info )
		g_free(info);
	length = g_list_length( list );
	info = g_new(GnomeUIInfo, length + 2);
	for (i = 0; i < length; i++) {
		info[i] = singleton;
		info[i].label = _(list->data);
		list = list->next;
	}
	info[i] = singleton;
	info[i].label = N_("Other...");
	i++;
	info[i] = end;

	*infop = info;
}

static void
_dialog_clicked(GtkWidget *dialog, gint button, EContactEditor *editor)
{
	GtkWidget *label = gtk_object_get_data(GTK_OBJECT(dialog),
					       "e_contact_editor_label");
#if 0
	GtkWidget *entry = gtk_object_get_data(GTK_OBJECT(dialog),
					       "e_contact_editor_entry");
#endif
	GtkWidget *dialog_entry = gtk_object_get_data(GTK_OBJECT(dialog),
						      "e_contact_editor_dialog_entry");
	GList **list = gtk_object_get_data(GTK_OBJECT(dialog),
					   "e_contact_editor_list");
	GList **info = gtk_object_get_data(GTK_OBJECT(dialog),
					   "e_contact_editor_info");
	switch (button) {
	case 0:
		gtk_object_set(GTK_OBJECT(label),
			       "label", gtk_entry_get_text(GTK_ENTRY(dialog_entry)),
			       NULL);
		*list = g_list_append(*list, g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog_entry))));
		g_free(*info);
		*info = NULL;
		break;
	}
	gnome_dialog_close(GNOME_DIALOG(dialog));
}

static void
_dialog_destroy(EContactEditor *editor, GtkWidget *dialog)
{
	gnome_dialog_close(GNOME_DIALOG(dialog));
}

static GtkWidget *
e_contact_editor_build_dialog(EContactEditor *editor, gchar *entry_id, gchar *label_id, gchar *title, GList **list, GnomeUIInfo **info)
{
	GtkWidget *dialog_entry = gtk_entry_new();
	GtkWidget *entry = glade_xml_get_widget(editor->gui, entry_id);
	GtkWidget *label = glade_xml_get_widget(editor->gui, label_id);
	
	GtkWidget *dialog = gnome_dialog_new(title,
					     NULL);

	gtk_container_add(GTK_CONTAINER(GNOME_DIALOG(dialog)->vbox),
			  gtk_widget_new (gtk_frame_get_type(),
					  "border_width", 4,
					  "label", title,
					  "child", gtk_widget_new(gtk_alignment_get_type(),
								  "child", dialog_entry,
								  "xalign", .5,
								  "yalign", .5,
								  "xscale", 1.0,
								  "yscale", 1.0,
								  "border_width", 9,
								  NULL),
					  NULL));

	gnome_dialog_append_button_with_pixmap(GNOME_DIALOG(dialog),
					       "Add",
					       GNOME_STOCK_PIXMAP_ADD);
	gnome_dialog_append_button(GNOME_DIALOG(dialog), GNOME_STOCK_BUTTON_CANCEL);
	gnome_dialog_set_default(GNOME_DIALOG(dialog), 0);
	
	gtk_signal_connect(GTK_OBJECT(dialog), "clicked",
			   _dialog_clicked, editor);
	gtk_signal_connect_while_alive(GTK_OBJECT(editor), "destroy",
				       _dialog_destroy, GTK_OBJECT(dialog), GTK_OBJECT(dialog));
	
	gtk_object_set_data(GTK_OBJECT(dialog),
			    "e_contact_editor_entry", entry);
	gtk_object_set_data(GTK_OBJECT(dialog),
			    "e_contact_editor_label", label);
	gtk_object_set_data(GTK_OBJECT(dialog),
			    "e_contact_editor_dialog_entry", dialog_entry);
	gtk_object_set_data(GTK_OBJECT(dialog),
			    "e_contact_editor_list", list);
	gtk_object_set_data(GTK_OBJECT(dialog),
			    "e_contact_editor_info", info);

	gtk_widget_show_all(dialog);
	return dialog;
}

static void
_phone_arrow_pressed (GtkWidget *widget, GdkEventButton *button, EContactEditor *editor)
{
	int which;
	int i;
	gchar *label;
	gchar *entry;
	if ( widget == glade_xml_get_widget(editor->gui, "button-phone1") ) {
		which = 1;
	} else if ( widget == glade_xml_get_widget(editor->gui, "button-phone2") ) {
		which = 2;
	} else if ( widget == glade_xml_get_widget(editor->gui, "button-phone3") ) {
		which = 3;
	} else if ( widget == glade_xml_get_widget(editor->gui, "button-phone4") ) {
		which = 4;
	} else
		return;
	
	label = g_strdup_printf("label-phone%d", which);
	entry = g_strdup_printf("entry-phone%d", which);

	if (editor->phone_list == NULL) {
		static char *info[] = {
			N_("Assistant"),
			N_("Business"),
			N_("Business 2"),
			N_("Business Fax"),
			N_("Callback"),
			N_("Car"),
			N_("Company"),
			N_("Home"),
			N_("Home 2"),
			N_("Home Fax"),
			N_("ISDN"),
			N_("Mobile"),
			N_("Other"),
			N_("Other Fax"),
			N_("Pager"),
			N_("Primary"),
			N_("Radio"),
			N_("Telex"),
			N_("TTY/TDD")
		};
		
		for (i = 0; i < sizeof(info) / sizeof(info[0]); i++) {
			editor->phone_list = g_list_append(editor->phone_list, g_strdup(info[i]));
		}
	}
	if (editor->phone_info == NULL) {
		e_contact_editor_build_ui_info(editor->phone_list, &editor->phone_info);
		
		if ( editor->phone_popup )
			gtk_widget_unref(editor->phone_popup);
		
		editor->phone_popup = gnome_popup_menu_new(editor->phone_info);
	}
	
	_arrow_pressed (widget, button, editor, editor->phone_popup, &editor->phone_list, &editor->phone_info, label, entry, "Add new phone number type");

	g_free(label);
	g_free(entry);
}

static void
_email_arrow_pressed (GtkWidget *widget, GdkEventButton *button, EContactEditor *editor)
{
	int i;
	if (editor->email_list == NULL) {
		static char *info[] = {
			N_("Email"),
			N_("Email 2"),
			N_("Email 3")
		};
		
		for (i = 0; i < sizeof(info) / sizeof(info[0]); i++) {
			editor->email_list = g_list_append(editor->email_list, g_strdup(info[i]));
		}
	}
	if (editor->email_info == NULL) {
		e_contact_editor_build_ui_info(editor->email_list, &editor->email_info);

		if ( editor->email_popup )
			gtk_widget_unref(editor->email_popup);
		
		editor->email_popup = gnome_popup_menu_new(editor->email_info);
	}
	
	_arrow_pressed (widget, button, editor, editor->email_popup, &editor->email_list, &editor->email_info, "label-email1", "entry-email1", "Add new Email type");
}

static void
_address_arrow_pressed (GtkWidget *widget, GdkEventButton *button, EContactEditor *editor)
{
	int i;
	if (editor->address_list == NULL) {
		static char *info[] = {
			N_("Business"),
			N_("Home"),
			N_("Other")
		};
		
		for (i = 0; i < sizeof(info) / sizeof(info[0]); i++) {
			editor->address_list = g_list_append(editor->address_list, g_strdup(info[i]));
		}
	}
	if (editor->address_info == NULL) {
		e_contact_editor_build_ui_info(editor->address_list, &editor->address_info);

		if ( editor->address_popup )
			gtk_widget_unref(editor->address_popup);
		
		editor->address_popup = gnome_popup_menu_new(editor->address_info);
	}
	
	_arrow_pressed (widget, button, editor, editor->address_popup, &editor->address_list, &editor->address_info, "label-address1", "entry-address1", "Add new Address type");
}
