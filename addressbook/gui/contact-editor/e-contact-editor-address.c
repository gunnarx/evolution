/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * e-contact-editor-address.c
 * Copyright (C) 2000  Ximian, Inc.
 * Author: Chris Lahey <clahey@ximian.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
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

#include <config.h>

#include <e-contact-editor-address.h>

#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-stock.h>
#include <gal/widgets/e-unicode.h>
#include <gal/widgets/e-gui-utils.h>
#include <gtk/gtkcombo.h>
#include <string.h>
#include <stdlib.h>

static void e_contact_editor_address_init		(EContactEditorAddress		 *card);
static void e_contact_editor_address_class_init	(EContactEditorAddressClass	 *klass);
static void e_contact_editor_address_set_arg (GtkObject *o, GtkArg *arg, guint arg_id);
static void e_contact_editor_address_get_arg (GtkObject *object, GtkArg *arg, guint arg_id);
static void e_contact_editor_address_destroy (GtkObject *object);

static void fill_in_info(EContactEditorAddress *editor);
static void extract_info(EContactEditorAddress *editor);

static GnomeDialogClass *parent_class = NULL;

/* The arguments we take */
enum {
	ARG_0,
	ARG_ADDRESS,
	ARG_IS_READ_ONLY
};

GtkType
e_contact_editor_address_get_type (void)
{
	static GtkType contact_editor_address_type = 0;

	if (!contact_editor_address_type)
		{
			static const GtkTypeInfo contact_editor_address_info =
			{
				"EContactEditorAddress",
				sizeof (EContactEditorAddress),
				sizeof (EContactEditorAddressClass),
				(GtkClassInitFunc) e_contact_editor_address_class_init,
				(GtkObjectInitFunc) e_contact_editor_address_init,
				/* reserved_1 */ NULL,
				/* reserved_2 */ NULL,
				(GtkClassInitFunc) NULL,
			};

			contact_editor_address_type = gtk_type_unique (gnome_dialog_get_type (), &contact_editor_address_info);
		}

	return contact_editor_address_type;
}

static void
e_contact_editor_address_class_init (EContactEditorAddressClass *klass)
{
	GtkObjectClass *object_class;
	GnomeDialogClass *dialog_class;

	object_class = (GtkObjectClass*) klass;
	dialog_class = (GnomeDialogClass *) klass;

	parent_class = gtk_type_class (gnome_dialog_get_type ());

	gtk_object_add_arg_type ("EContactEditorAddress::address", GTK_TYPE_POINTER, 
				 GTK_ARG_READWRITE, ARG_ADDRESS);
	gtk_object_add_arg_type ("EContactEditorAddress::editable", GTK_TYPE_BOOL, 
				 GTK_ARG_READWRITE, ARG_IS_READ_ONLY);
 
	object_class->set_arg = e_contact_editor_address_set_arg;
	object_class->get_arg = e_contact_editor_address_get_arg;
	object_class->destroy = e_contact_editor_address_destroy;
}

static GList *
add_to_tab_order(GList *list, GladeXML *gui, char *name)
{
	GtkWidget *widget = glade_xml_get_widget(gui, name);
	return g_list_prepend(list, widget);
}

static void
setup_tab_order(GladeXML *gui)
{
	GtkWidget *container;
	GList *list = NULL;

	container = glade_xml_get_widget(gui, "table-checkaddress");

	if (container) {
		list = add_to_tab_order(list, gui, "entry-city");
		list = add_to_tab_order(list, gui, "entry-region");
		list = add_to_tab_order(list, gui, "entry-code");
		list = add_to_tab_order(list, gui, "combo-country");
		list = g_list_reverse(list);
		e_container_change_tab_order(GTK_CONTAINER(container), list);
		g_list_free(list);
	}
}

static char * countries [] = {
	N_("United States"),
	N_("Afghanistan"),
	N_("Albania"),
	N_("Algeria"),
	N_("American Samoa"),
	N_("Andorra"),
	N_("Angola"),
	N_("Anguilla"),
	N_("Antarctica"),
	N_("Antigua And Barbuda"),
	N_("Argentina"),
	N_("Armenia"),
	N_("Aruba"),
	N_("Australia"),
	N_("Austria"),
	N_("Azerbaijan"),
	N_("Bahamas"),
	N_("Bahrain"),
	N_("Bangladesh"),
	N_("Barbados"),
	N_("Belarus"),
	N_("Belgium"),
	N_("Belize"),
	N_("Benin"),
	N_("Bermuda"),
	N_("Bhutan"),
	N_("Bolivia"),
	N_("Bosnia And Herzegowina"),
	N_("Botswana"),
	N_("Bouvet Island"),
	N_("Brazil"),
	N_("British Indian Ocean Territory"),
	N_("Brunei Darussalam"),
	N_("Bulgaria"),
	N_("Burkina Faso"),
	N_("Burundi"),
	N_("Cambodia"),
	N_("Cameroon"),
	N_("Canada"),
	N_("Cape Verde"),
	N_("Cayman Islands"),
	N_("Central African Republic"),
	N_("Chad"),
	N_("Chile"),
	N_("China"),
	N_("Christmas Island"),
	N_("Cocos (Keeling) Islands"),
	N_("Colombia"),
	N_("Comoros"),
	N_("Congo"),
	N_("Congo"),
	N_("Cook Islands"),
	N_("Costa Rica"),
	N_("Cote d'Ivoire"),
	N_("Croatia"),
	N_("Cuba"),
	N_("Cyprus"),
	N_("Czech Republic"),
	N_("Denmark"),
	N_("Djibouti"),
	N_("Dominica"),
	N_("Dominican Republic"),
	N_("East Timor"),
	N_("Ecuador"),
	N_("Egypt"),
	N_("El Salvador"),
	N_("Equatorial Guinea"),
	N_("Eritrea"),
	N_("Estonia"),
	N_("Ethiopia"),
	N_("Falkland Islands"),
	N_("Faroe Islands"),
	N_("Fiji"),
	N_("Finland"),
	N_("France"),
	N_("French Guiana"),
	N_("French Polynesia"),
	N_("French Southern Territories"),
	N_("Gabon"),
	N_("Gambia"),
	N_("Georgia"),
	N_("Germany"),
	N_("Ghana"),
	N_("Gibraltar"),
	N_("Greece"),
	N_("Greenland"),
	N_("Grenada"),
	N_("Guadeloupe"),
	N_("Guam"),
	N_("Guatemala"),
	N_("Guinea"),
	N_("Guinea-bissau"),
	N_("Guyana"),
	N_("Haiti"),
	N_("Heard And McDonald Islands"),
	N_("Holy See"),
	N_("Honduras"),
	N_("Hong Kong"),
	N_("Hungary"),
	N_("Iceland"),
	N_("India"),
	N_("Indonesia"),
	N_("Ireland"),
	N_("Israel"),
	N_("Italy"),
	N_("Jamaica"),
	N_("Japan"),
	N_("Jordan"),
	N_("Kazakhstan"),
	N_("Kenya"),
	N_("Kiribati"),
	N_("Korea, Republic Of"),
	N_("Kuwait"),
	N_("Kyrgyzstan"),
	N_("Laos"),
	N_("Latvia"),
	N_("Lebanon"),
	N_("Lesotho"),
	N_("Liberia"),
	N_("Liechtenstein"),
	N_("Lithuania"),
	N_("Luxembourg"),
	N_("Macau"),
	N_("Macedonia"),
	N_("Madagascar"),
	N_("Malawi"),
	N_("Malaysia"),
	N_("Maldives"),
	N_("Mali"),
	N_("Malta"),
	N_("Marshall Islands"),
	N_("Martinique"),
	N_("Mauritania"),
	N_("Mauritius"),
	N_("Mayotte"),
	N_("Mexico"),
	N_("Micronesia"),
	N_("Moldova, Republic Of"),
	N_("Monaco"),
	N_("Mongolia"),
	N_("Montserrat"),
	N_("Morocco"),
	N_("Mozambique"),
	N_("Myanmar"),
	N_("Namibia"),
	N_("Nauru"),
	N_("Nepal"),
	N_("Netherlands"),
	N_("Netherlands Antilles"),
	N_("New Caledonia"),
	N_("New Zealand"),
	N_("Nicaragua"),
	N_("Niger"),
	N_("Nigeria"),
	N_("Niue"),
	N_("Norfolk Island"),
	N_("Northern Mariana Islands"),
	N_("Norway"),
	N_("Oman"),
	N_("Pakistan"),
	N_("Palau"),
	N_("Palestinian Territory"),
	N_("Panama"),
	N_("Papua New Guinea"),
	N_("Paraguay"),
	N_("Peru"),
	N_("Philippines"),
	N_("Pitcairn"),
	N_("Poland"),
	N_("Portugal"),
	N_("Puerto Rico"),
	N_("Qatar"),
	N_("Reunion"),
	N_("Romania"),
	N_("Russian Federation"),
	N_("Rwanda"),
	N_("Saint Kitts And Nevis"),
	N_("Saint Lucia"),
	N_("Saint Vincent And The Grena-dines"),
	N_("Samoa"),
	N_("San Marino"),
	N_("Sao Tome And Principe"),
	N_("Saudi Arabia"),
	N_("Senegal"),
	N_("Seychelles"),
	N_("Sierra Leone"),
	N_("Singapore"),
	N_("Slovakia"),
	N_("Slovenia"),
	N_("Solomon Islands"),
	N_("Somalia"),
	N_("South Africa"),
	N_("South Georgia And The South Sandwich Islands"),
	N_("Spain"),
	N_("Sri Lanka"),
	N_("St. Helena"),
	N_("St. Pierre And Miquelon"),
	N_("Sudan"),
	N_("Suriname"),
	N_("Svalbard And Jan Mayen Islands"),
	N_("Swaziland"),
	N_("Sweden"),
	N_("Switzerland"),
	N_("Taiwan"),
	N_("Tajikistan"),
	N_("Tanzania, United Republic Of"),
	N_("Thailand"),
	N_("Togo"),
	N_("Tokelau"),
	N_("Tonga"),
	N_("Trinidad And Tobago"),
	N_("Tunisia"),
	N_("Turkey"),
	N_("Turkmenistan"),
	N_("Turks And Caicos Islands"),
	N_("Tuvalu"),
	N_("Uganda"),
	N_("Ukraine"),
	N_("United Arab Emirates"),
	N_("United Kingdom"),
	N_("United States Minor Outlying Islands"),
	N_("Uruguay"),
	N_("Uzbekistan"),
	N_("Vanuatu"),
	N_("Venezuela"),
	N_("Viet Nam"),
	N_("Virgin Islands, British"),
	N_("Virgin Islands, U.S."),
	N_("Wallis And Futuna Islands"),
	N_("Western Sahara"),
	N_("Yemen"),
	N_("Yugoslavia"),
	N_("Zambia"),
	N_("Zimbabwe"),
	NULL
};

static int
compare_func (const void *voida, const void *voidb)
{
	char * const *stringa = voida, * const *stringb = voidb;

	return strcoll (*stringa, *stringb);
}

static void
fill_in_countries (GladeXML *gui)
{
	GtkCombo *combo;
	combo = (GtkCombo *) glade_xml_get_widget(gui, "combo-country");
	if (combo && GTK_IS_COMBO (combo)) {
		static gboolean sorted = FALSE;
		static GList *country_list;
		if (!sorted) {
			int i;
			char *locale;

			for (i = 0; countries[i]; i++) {
				countries[i] = _(countries[i]);
			}

			locale = setlocale (LC_COLLATE, NULL);
			qsort (countries + 1, i - 1, sizeof (countries[0]), compare_func);
			country_list = NULL;
			for (i = 0; countries[i]; i++) {
				country_list = g_list_prepend (country_list, countries[i]);
			}
			country_list = g_list_reverse (country_list);
			sorted = TRUE;
		}
		gtk_combo_set_popdown_strings (combo, country_list);
	}
}

static void
e_contact_editor_address_init (EContactEditorAddress *e_contact_editor_address)
{
	GladeXML *gui;
	GtkWidget *widget;

	gnome_dialog_append_button ( GNOME_DIALOG(e_contact_editor_address),
				     GNOME_STOCK_BUTTON_OK);
	
	gnome_dialog_append_button ( GNOME_DIALOG(e_contact_editor_address),
				     GNOME_STOCK_BUTTON_CANCEL);

	gtk_window_set_policy(GTK_WINDOW(e_contact_editor_address), FALSE, TRUE, FALSE);

	e_contact_editor_address->address = NULL;

	gui = glade_xml_new (EVOLUTION_GLADEDIR "/fulladdr.glade", NULL);
	e_contact_editor_address->gui = gui;

	setup_tab_order (gui);
	fill_in_countries (gui);

	widget = glade_xml_get_widget(gui, "table-checkaddress");
	gtk_widget_ref(widget);
	gtk_container_remove(GTK_CONTAINER(widget->parent), widget);
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (e_contact_editor_address)->vbox), widget, TRUE, TRUE, 0);
	gtk_widget_unref(widget);
}

void
e_contact_editor_address_destroy (GtkObject *object)
{
	EContactEditorAddress *e_contact_editor_address = E_CONTACT_EDITOR_ADDRESS(object);

	if (e_contact_editor_address->gui)
		gtk_object_unref(GTK_OBJECT(e_contact_editor_address->gui));
	e_card_delivery_address_unref(e_contact_editor_address->address);
}

GtkWidget*
e_contact_editor_address_new (const ECardDeliveryAddress *address)
{
	GtkWidget *widget = GTK_WIDGET (gtk_type_new (e_contact_editor_address_get_type ()));
	gtk_object_set (GTK_OBJECT(widget),
			"address", address,
			NULL);
	return widget;
}

static void
e_contact_editor_address_set_arg (GtkObject *o, GtkArg *arg, guint arg_id)
{
	EContactEditorAddress *e_contact_editor_address;

	e_contact_editor_address = E_CONTACT_EDITOR_ADDRESS (o);
	
	switch (arg_id){
	case ARG_ADDRESS:
		e_card_delivery_address_unref(e_contact_editor_address->address);
		e_contact_editor_address->address = e_card_delivery_address_copy(GTK_VALUE_POINTER (*arg));
		fill_in_info(e_contact_editor_address);
		break;
	case ARG_IS_READ_ONLY: {
		int i;
		char *entry_names[] = {
			"entry-street",
			"entry-city",
			"entry-ext",
			"entry-po",
			"entry-region",
			"combo-country",
			"entry-code",
			NULL
		};
		e_contact_editor_address->editable = GTK_VALUE_BOOL (*arg) ? TRUE : FALSE;
		for (i = 0; entry_names[i] != NULL; i ++) {
			GtkWidget *w = glade_xml_get_widget(e_contact_editor_address->gui, entry_names[i]);
			if (GTK_IS_ENTRY (w)) {
				gtk_entry_set_editable (GTK_ENTRY (w),
							e_contact_editor_address->editable);
			}
			else if (GTK_IS_COMBO (w)) {
				gtk_entry_set_editable (GTK_ENTRY (GTK_COMBO (w)->entry),
							e_contact_editor_address->editable);
				gtk_widget_set_sensitive (GTK_COMBO (w)->button, e_contact_editor_address->editable);
			}
		}
		break;
	}
	}
}

static void
e_contact_editor_address_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	EContactEditorAddress *e_contact_editor_address;

	e_contact_editor_address = E_CONTACT_EDITOR_ADDRESS (object);

	switch (arg_id) {
	case ARG_ADDRESS:
		extract_info(e_contact_editor_address);
		GTK_VALUE_POINTER (*arg) = e_card_delivery_address_ref(e_contact_editor_address->address);
		break;
	case ARG_IS_READ_ONLY:
		GTK_VALUE_BOOL (*arg) = e_contact_editor_address->editable ? TRUE : FALSE;
		break;
	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
fill_in_field(EContactEditorAddress *editor, char *field, char *string)
{
	GtkEditable *editable = GTK_EDITABLE(glade_xml_get_widget(editor->gui, field));
	if (editable) {
		e_utf8_gtk_editable_set_text(editable, string);
	}
}

static void
fill_in_info(EContactEditorAddress *editor)
{
	ECardDeliveryAddress *address = editor->address;
	if (address) {
		fill_in_field(editor, "entry-street" , address->street );
		fill_in_field(editor, "entry-po"     , address->po     );
		fill_in_field(editor, "entry-ext"    , address->ext    );
		fill_in_field(editor, "entry-city"   , address->city   );
		fill_in_field(editor, "entry-region" , address->region );
		fill_in_field(editor, "entry-code"   , address->code   );
		fill_in_field(editor, "entry-country", address->country);
	}
}

static char *
extract_field(EContactEditorAddress *editor, char *field)
{
	GtkEditable *editable = GTK_EDITABLE(glade_xml_get_widget(editor->gui, field));
	if (editable)
		return e_utf8_gtk_editable_get_text(editable);
	else
		return NULL;
}

static void
extract_info(EContactEditorAddress *editor)
{
	ECardDeliveryAddress *address = editor->address;
	if (!address) {
		address = e_card_delivery_address_new();
		editor->address = address;
	}
	address->street  = extract_field(editor, "entry-street" );
	address->po      = extract_field(editor, "entry-po"     );
	address->ext     = extract_field(editor, "entry-ext"    );
	address->city    = extract_field(editor, "entry-city"   );
	address->region  = extract_field(editor, "entry-region" );
	address->code    = extract_field(editor, "entry-code"   );
	address->country = extract_field(editor, "entry-country");
}
