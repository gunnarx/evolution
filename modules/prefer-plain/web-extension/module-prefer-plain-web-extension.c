/*
 * module-prefer-plain-web-extension.c
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#include "module-prefer-plain-web-extension.h"

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <webkit2/webkit-web-extension.h>

/* FIXME Clean it */
static GDBusConnection *dbus_connection;

static const char introspection_xml[] =
"<node>"
"  <interface name='org.gnome.Evolution.Module.PreferPlain.WebExtension'>"
"    <method name='ChangeIFrameSource'>"
"      <arg type='s' name='new_uri' direction='in'/>"
"    </method>"
"    <method name='SaveDocumentFromPoint'>"
"      <arg type='t' name='page_id' direction='in'/>"
"      <arg type='i' name='x' direction='in'/>"
"      <arg type='i' name='y' direction='in'/>"
"    </method>"
"    <method name='GetDocumentURI'>"
"      <arg type='s' name='document_uri' direction='out'/>"
"    </method>"
"  </interface>"
"</node>";

static WebKitDOMDocument *document_saved = NULL;

static WebKitWebPage *
get_webkit_web_page_or_return_dbus_error (GDBusMethodInvocation *invocation,
                                          WebKitWebExtension *web_extension,
                                          guint64 page_id)
{
	WebKitWebPage *web_page = webkit_web_extension_get_page (web_extension, page_id);
	if (!web_page) {
		g_dbus_method_invocation_return_error (
			invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
			"Invalid page ID: %"G_GUINT64_FORMAT, page_id);
	}
	return web_page;
}

static void
handle_method_call (GDBusConnection *connection,
                    const char *sender,
                    const char *object_path,
                    const char *interface_name,
                    const char *method_name,
                    GVariant *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer user_data)
{
	WebKitWebExtension *web_extension = WEBKIT_WEB_EXTENSION (user_data);
	WebKitWebPage *web_page;
	WebKitDOMDocument *document;
	guint64 page_id;

	if (g_strcmp0 (interface_name, MODULE_PREFER_PLAIN_WEB_EXTENSION_INTERFACE) != 0)
		return;

	if (g_strcmp0 (method_name, "ChangeIFrameSource") == 0) {
		WebKitDOMDOMWindow *window;
		WebKitDOMElement *frame_element;
		const gchar *new_uri;

		g_variant_get (parameters, "(t&s)", &page_id, &new_uri);

		/* Get frame's window and from the window the actual <iframe> element */
		window = webkit_dom_document_get_default_view (document_saved);
		frame_element = webkit_dom_dom_window_get_frame_element (window);
		webkit_dom_html_iframe_element_set_src (
			WEBKIT_DOM_HTML_IFRAME_ELEMENT (frame_element), new_uri);

		g_dbus_method_invocation_return_value (invocation, NULL);
	} else if (g_strcmp0 (method_name, "SaveDocumentFromPoint") == 0) {
		gint32 x = 0, y = 0;
		WebKitDOMElement *active_element;

		g_variant_get (parameters, "(tii)", &page_id, &x, &y);
		web_page = get_webkit_web_page_or_return_dbus_error (invocation, web_extension, page_id);
		if (!web_page)
			return;

		document = webkit_web_page_get_dom_document (web_page);
		document_saved = document;

		if (x == 0 && y == 0)
			active_element = webkit_dom_html_document_get_active_element (WEBKIT_DOM_HTML_DOCUMENT (document));
		else
			active_element = webkit_dom_document_element_from_point (document, x,y);

		if (WEBKIT_DOM_IS_HTML_IFRAME_ELEMENT (active_element)) {
			document_saved =
				webkit_dom_html_iframe_element_get_content_document (
					WEBKIT_DOM_HTML_IFRAME_ELEMENT (active_element));
		}

		g_dbus_method_invocation_return_value (invocation, NULL);
	} else if (g_strcmp0 (method_name, "GetDocumentURI") == 0) {
		gchar *document_uri;

		document_uri = webkit_dom_document_get_document_uri (document_saved);

		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(s)", document_uri));
	}
}

static const GDBusInterfaceVTable interface_vtable = {
	handle_method_call,
	NULL,
	NULL
};

static void
bus_acquired_cb (GDBusConnection *connection,
                 const char *name,
                 gpointer user_data)
{
	guint registration_id;
	GError *error = NULL;
	static GDBusNodeInfo *introspection_data = NULL;

	if (!introspection_data)
		introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);

	registration_id =
		g_dbus_connection_register_object (
			connection,
			MODULE_PREFER_PLAIN_WEB_EXTENSION_OBJECT_PATH,
			introspection_data->interfaces[0],
			&interface_vtable,
			g_object_ref (user_data),
			g_object_unref,
			&error);

	if (!registration_id) {
		g_warning ("Failed to register object: %s\n", error->message);
		g_error_free (error);
	} else {
		dbus_connection = connection;
		g_object_add_weak_pointer (G_OBJECT (connection), (gpointer *)&dbus_connection);
	}
}

/* Forward declaration */
G_MODULE_EXPORT void webkit_web_extension_initialize (WebKitWebExtension *extension);

G_MODULE_EXPORT void
webkit_web_extension_initialize (WebKitWebExtension *extension)
{
	char *service_name;

	service_name =
		g_strdup_printf (
			"%s-%s",
			MODULE_PREFER_PLAIN_WEB_EXTENSION_SERVICE_NAME,
			g_getenv ("MODULE_PREFER_PLAIN_WEB_EXTENSION_ID"));

	g_bus_own_name (
		G_BUS_TYPE_SESSION,
		service_name,
		G_BUS_NAME_OWNER_FLAGS_NONE,
		bus_acquired_cb,
		NULL, NULL,
		g_object_ref (extension),
		g_object_unref);

	g_free (service_name);
}