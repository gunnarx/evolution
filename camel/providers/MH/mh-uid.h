/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* 
 *
 * Copyright (C) 1999 Bertrand Guiheneuf <Bertrand.Guiheneuf@aful.org> .
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef MH_UID_H
#define MH_UID_H 1

#include <glib.h>
#include "camel-mh-folder.h"


typedef struct {
	gchar uid[16];
	guint file_number;
} MhUidCouple;

void mh_uid_get_for_file (gchar *filename, guchar uid[16]);
void mh_save_uid_list (CamelMhFolder *mh_folder);
gint mh_load_uid_list (CamelMhFolder *mh_folder);
gint mh_generate_uid_list (CamelMhFolder *mh_folder);

#endif /* MH_UID_H */
