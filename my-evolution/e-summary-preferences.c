/* 
 * e-summary-preferences.c:
 *
 * Copyright (C) 2001 Ximian, Inc
 *
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#include "e-summary.h"
#include "e-summary-preferences.h"

#include <gtk/gtk.h>

#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-config.h>

#include <libgnomeui/gnome-propertybox.h>
#include <libgnomeui/gnome-stock.h>

#include <glade/glade.h>
static void
make_initial_mail_list (ESummaryPrefs *prefs)
{
	char *evolution_dir;
	GList *folders;

	evolution_dir = gnome_util_prepend_user_home ("evolution/local");

	folders = g_list_append (NULL, g_strconcat (evolution_dir, "/Inbox", NULL));
	folders = g_list_append (folders, g_strconcat (evolution_dir, "/Outbox", NULL));

	g_free (evolution_dir);
	prefs->display_folders = folders;
}

static void
make_initial_rdf_list (ESummaryPrefs *prefs)
{
	GList *rdfs;

	rdfs = g_list_prepend (NULL, g_strdup ("http://news.gnome.org/gnome-news/rdf"));

	prefs->rdf_urls = rdfs;
}

static void
make_initial_weather_list (ESummaryPrefs *prefs)
{
	GList *stations;

	stations = g_list_prepend (NULL, g_strdup ("EGAA"));
	stations = g_list_prepend (stations, g_strdup ("EGAC"));
	stations = g_list_prepend (stations, g_strdup ("ENBR"));

	prefs->stations = stations;
}

/* Load the prefs off disk */

static char *
vector_from_str_list (GList *strlist)
{
	char *vector;
	GString *str;

	g_return_val_if_fail (strlist != NULL, NULL);

	str = g_string_new ("");
	for (; strlist; strlist = strlist->next) {
		g_string_append (str, strlist->data);

		/* No space at end */
		if (strlist->next) {
			g_string_append (str, " ");
		}
	}

	vector = str->str;
	g_string_free (str, FALSE);

	return vector;
}

static GList *
str_list_from_vector (const char *vector)
{
	GList *strlist = NULL;
	char **tokens, **t;

	t = tokens = g_strsplit (vector, " ", 8196);

	if (tokens == NULL) {
		return NULL;
	}

	for (; *tokens; tokens++) {
		strlist = g_list_prepend (strlist, g_strdup (*tokens));
	}

	g_strfreev (t);

	strlist = g_list_reverse (strlist);
	return strlist;
}

gboolean
e_summary_preferences_restore (ESummaryPrefs *prefs)
{
	char *evolution_dir, *key;
	char *vector;

	g_return_val_if_fail (prefs != NULL, FALSE);

	evolution_dir = gnome_util_prepend_user_home ("evolution/config/my-evolution");
	if (g_file_exists (evolution_dir) == FALSE) {
		g_free (evolution_dir);
		return FALSE;
	}

	key = g_strdup_printf ("=%s=/Mail/", evolution_dir);
	gnome_config_push_prefix (key);
	g_free (key);

	vector = gnome_config_get_string ("display_folders");
	prefs->display_folders = str_list_from_vector (vector);
	g_free (vector);

	prefs->show_full_path = gnome_config_get_bool ("show_full_path=FALSE");

	gnome_config_pop_prefix ();
	key = g_strdup_printf ("=%s=/RDF/", evolution_dir);
	gnome_config_push_prefix (key);
	g_free (key);

	vector = gnome_config_get_string ("rdf_urls");
	prefs->rdf_urls = str_list_from_vector (vector);
	g_free (vector);

	prefs->rdf_refresh_time = gnome_config_get_int ("rdf_refresh_time=600");
	prefs->limit = gnome_config_get_int ("limit=10");
	prefs->wipe_trackers = gnome_config_get_bool ("wipe_trackers=True");

	gnome_config_pop_prefix ();
	key = g_strdup_printf ("=%s=/Weather/", evolution_dir);
	gnome_config_push_prefix (key);
	g_free (key);

	vector = gnome_config_get_string ("stations");
	prefs->stations = str_list_from_vector (vector);
	g_free (vector);

	prefs->units = gnome_config_get_int ("units");
	prefs->weather_refresh_time = gnome_config_get_int ("weather_refresh_time");
	
	gnome_config_pop_prefix ();
	key = g_strdup_printf ("=%s=/Schedule/", evolution_dir);
	gnome_config_push_prefix (key);
	g_free (key);

	prefs->days = gnome_config_get_int ("days");
	prefs->show_tasks = gnome_config_get_int ("show_tasks");

	gnome_config_pop_prefix ();
	g_free (evolution_dir);
	return TRUE;
}

/* Write prefs to disk */
void
e_summary_preferences_save (ESummaryPrefs *prefs)
{
	char *evolution_dir, *key;
	char *vector;

	g_return_if_fail (prefs != NULL);

	g_print ("Saving stuff\n");
	evolution_dir = gnome_util_prepend_user_home ("evolution/config/my-evolution");
	key = g_strdup_printf ("=%s=/Mail/", evolution_dir);
	gnome_config_push_prefix (key);
	g_free (key);

	vector = vector_from_str_list (prefs->display_folders);
	gnome_config_set_string ("display_folders", vector);
	g_free (vector);

	gnome_config_set_bool ("show_full_path", prefs->show_full_path);

	gnome_config_pop_prefix ();
	key = g_strdup_printf ("=%s=/RDF/", evolution_dir);
	gnome_config_push_prefix (key);
	g_free (key);
	
	vector = vector_from_str_list (prefs->rdf_urls);
	gnome_config_set_string ("rdf_urls", vector);
	g_free (vector);

	gnome_config_set_int ("rdf_refresh_time", prefs->rdf_refresh_time);
	gnome_config_set_int ("limit", prefs->limit);
	gnome_config_set_bool ("wipe_trackers", prefs->wipe_trackers);

	gnome_config_pop_prefix ();
	key = g_strdup_printf ("=%s=/Weather/", evolution_dir);
	gnome_config_push_prefix (key);
	g_free (key);

	vector = vector_from_str_list (prefs->stations);
	gnome_config_set_string ("stations", vector);
	g_free (vector);

	gnome_config_set_int ("units", prefs->units);
	gnome_config_set_int ("weather_refresh_time", prefs->weather_refresh_time);
	
	gnome_config_pop_prefix ();
	key = g_strdup_printf ("=%s=/Schedule/", evolution_dir);
	gnome_config_push_prefix (key);
	g_free (key);

	gnome_config_set_int ("days", prefs->days);
	gnome_config_set_int ("show_tasks", prefs->show_tasks);

	gnome_config_pop_prefix ();

	gnome_config_sync ();
	gnome_config_drop_all ();

	g_free (evolution_dir);
}

static void
free_str_list (GList *list)
{
	for (; list; list = list->next) {
		g_free (list->data);
	}
}

void
e_summary_preferences_free (ESummaryPrefs *prefs)
{
	if (prefs->display_folders) {
		free_str_list (prefs->display_folders);
		g_list_free (prefs->display_folders);
	}

	if (prefs->rdf_urls) {
		free_str_list (prefs->rdf_urls);
		g_list_free (prefs->rdf_urls);
	}

	if (prefs->stations) {
		free_str_list (prefs->stations);
		g_list_free (prefs->stations);
	}

	g_free (prefs);
}

static GList *
copy_str_list (GList *list)
{
	GList *list_copy = NULL;

	for (; list; list = list->next) {
		list_copy = g_list_prepend (list_copy, g_strdup (list->data));
	}

	list_copy = g_list_reverse (list_copy);
	return list_copy;
}

ESummaryPrefs *
e_summary_preferences_copy (ESummaryPrefs *prefs)
{
	ESummaryPrefs *prefs_copy;

	prefs_copy = g_new (ESummaryPrefs, 1);

	prefs_copy->display_folders = copy_str_list (prefs->display_folders);
	prefs_copy->show_full_path = prefs->show_full_path;

	prefs_copy->rdf_urls = copy_str_list (prefs->rdf_urls);
	prefs_copy->rdf_refresh_time = prefs->rdf_refresh_time;
	prefs_copy->limit = prefs->limit;
	prefs_copy->wipe_trackers = prefs->wipe_trackers;

	prefs_copy->stations = copy_str_list (prefs->stations);
	prefs_copy->units = prefs->units;
	prefs_copy->weather_refresh_time = prefs->weather_refresh_time;

	prefs_copy->days = prefs->days;
	prefs_copy->show_tasks = prefs->show_tasks;

	return prefs_copy;
}

void
e_summary_preferences_init (ESummary *summary)
{
	ESummaryPrefs *prefs;

	g_return_if_fail (summary != NULL);
	g_return_if_fail (IS_E_SUMMARY (summary));

	summary->preferences = g_new0 (ESummaryPrefs, 1);
	summary->old_prefs = NULL;

	if (e_summary_preferences_restore (summary->preferences) == TRUE) {
		return;
	}

	prefs = summary->preferences;
	/* Defaults */
	
	/* Mail */
	make_initial_mail_list (prefs);

	/* RDF */
	make_initial_rdf_list (prefs);
	prefs->rdf_refresh_time = 600;
	prefs->limit = 10;
	prefs->wipe_trackers = FALSE;

	/* Weather */
	make_initial_weather_list (prefs);
	prefs->units = UNITS_METRIC;
	prefs->weather_refresh_time = 600;

	prefs->days = E_SUMMARY_CALENDAR_ONE_DAY;
	prefs->show_tasks = E_SUMMARY_CALENDAR_ALL_TASKS;
}

struct _MailPage {
	GtkWidget *all, *shown;
	GtkWidget *fullpath;
	GtkWidget *add, *remove;
};

struct _RDFPage {
	GtkWidget *all, *shown;
	GtkWidget *refresh, *limit, *wipe_trackers;
	GtkWidget *add, *remove;
	GtkWidget *new_url;
};

struct _WeatherPage {
	GtkWidget *all, *shown;
	GtkWidget *refresh, *imperial, *metric;
	GtkWidget *add, *remove;

	GtkCTreeNode *selected_node;
};

struct _CalendarPage {
	GtkWidget *one, *five, *week, *month;
	GtkWidget *all, *today;
};

typedef struct _PropertyData {
	ESummary *summary;
	GnomePropertyBox *box;
	GtkWidget *new_entry;
	GladeXML *xml;

	struct _MailPage *mail;
	struct _RDFPage *rdf;
	struct _WeatherPage *weather;
	struct _CalendarPage *calendar;
} PropertyData;

static char *rdfs[] = {
	"http://news.gnome.org/gnome-news/rdf",
	"http://advogato.org/rss/articles.xml",
	"http://www.appwatch.com/appwatch.rdf",
	"http://barrapunto.com/barrapunto.rdf",
	"http://barrapunto.com/gnome.rdf",
	"http://www.bsdtoday.com/backend/bt.rdf",
	"http://beyond2000.com/b2k.rdf",
	"http://www.newsisfree.com/export.php3?_f=rss91&_w=f&_i=1443",
	"http://www.cnn.com/cnn.rss",
	"http://www.dictionary.com/wordoftheday/wotd.rss",
	"http://www.dvdreview.com/rss/newschannel.rss",
	"http://freshmeat.net/backend/fm.rdf",
	"http://headlines.internet.com/internetnews/prod-news/news.rss",
	"http://www.kde.org/news/kdenews.rdf",
	"http://www.kuro5hin.org/backend.rdf",
	"http://linuxgames.com/bin/mynetscape.pl",
	"http://linux.com/mrn/jobs/latest_jobs.rss",
	"http://www.linuxplanet.com/rss",
	"http://linuxtoday.com/backend/my-netscape.rdf",
	"http://lwn.net/headlines/rss",
	"http://www.linux.com/mrn/front_page.rss",
	"http://morons.org/morons.rss",
	"http://www.mozilla.org/news.rdf",
	"http://www.mozillazine.org/contents.rdf",
	"http://www.fool.com/about/headlines/rss_headlines.asp",
	"http://www.newsforge.com/newsforge.rss",
	"http://www.nanotechnews.com/nano/rdf",
	"http://www.perl.com/pace/news.rss",
	"http://www.pigdog.org/pigdog.rdf",
	"http://www.python.org/channews.rdf",
	"http://www.quotationspage.com/data/mqotd.rss",
	"http://www.salon.com/feed/RDF/salon_use.rdf",
	"http://www.scriptingnews.userland.com/xml/scriptingnews2.xml",
	"http://www.securityfocus.com/topnews-rss.html",
	"http://www.segfault.org/stories.xml",
	"http://www.slashdot.org/slashdot.rdf",
	"http://www.theregister.co.uk/tonys/slashdot.org",
	"http://www.thinkgeek.com/thinkgeek.rdf",
	"http://www.tomalak.org/recentTodaysLinks.xml",
	"http://www.webreference.com/webreference.rdf",
	"http://www.zope.org/SiteIndex/news.rss",
	NULL
};

static void
fill_rdf_all_clist (GtkCList *clist)
{
	int i;

	for (i = 0; rdfs[i]; i++) {
		char *text[1];

		text[0] = rdfs[i];
		gtk_clist_append (clist, text);
	}
}

static void
fill_rdf_shown_clist (GtkCList *clist,
		      PropertyData *pd)
{
	GList *p;

	for (p = pd->summary->preferences->rdf_urls; p; p = p->next) {
		char *text[1];

		text[0] = p->data;
		gtk_clist_append (clist, text);
	}
}

static void
fill_weather_all_ctree (GtkCTree *ctree)
{
	e_summary_weather_ctree_fill (ctree);
}

static void
fill_weather_shown_clist (GtkCList *clist,
			  PropertyData *pd)
{
	GList *p;

	for (p = pd->summary->preferences->stations; p; p = p->next) {
		char *text[1];
		char *pretty;

		pretty = (char *) e_summary_weather_code_to_name (p->data);

		text[0] = pretty;
		gtk_clist_append (clist, text);
	}
}

static void
fill_mail_shown_clist (GtkCList *clist,
		       PropertyData *pd)
{
	GList *p;

	for (p = pd->summary->preferences->display_folders; p; p = p->next) {
		char *text[1];

		text[0] = p->data;
		gtk_clist_append (clist, text);
	}
}

static void
mail_shown_select_row_cb (GtkCList *clist,
			  int row,
			  int column,
			  GdkEvent *event,
			  PropertyData *pd)
{
	gtk_widget_set_sensitive (pd->mail->remove, TRUE);
}

static void
mail_shown_unselect_row_cb (GtkCList *clist,
			    int row,
			    int column,
			    GdkEvent *event,
			    PropertyData *pd)
{
	if (clist->selection == NULL) {
		gtk_widget_set_sensitive (pd->mail->remove, FALSE);
	}
}

static void
mail_add_clicked_cb (GtkButton *button,
		     PropertyData *pd)
{
	
}

static void
mail_remove_clicked_cb (GtkButton *button,
			PropertyData *pd)
{
	int row;
	GList *p;

	row = GPOINTER_TO_INT (GTK_CLIST (pd->mail->shown)->selection->data);
	p = g_list_nth (pd->summary->preferences->display_folders, row);

	gtk_clist_remove (GTK_CLIST (pd->mail->shown), row);
	pd->summary->preferences->display_folders = g_list_remove_link (pd->summary->preferences->display_folders, p);
	g_free (p->data);
	g_list_free (p);

	gnome_property_box_changed (pd->box);
}

static void
mail_show_full_path_toggled_cb (GtkToggleButton *tb,
				PropertyData *pd)
{
	pd->summary->preferences->show_full_path = gtk_toggle_button_get_active (tb);
}

static void
rdf_all_select_row_cb (GtkCList *clist,
		       int row,
		       int column,
		       GdkEvent *event,
		       PropertyData *pd)
{
	gtk_widget_set_sensitive (pd->rdf->add, TRUE);
}

static void
rdf_all_unselect_row_cb (GtkCList *clist,
			 int row,
			 int column,
			 GdkEvent *event,
			 PropertyData *pd)
{
	if (GTK_CLIST (pd->rdf->all)->selection == NULL) {
		gtk_widget_set_sensitive (pd->rdf->add, FALSE);
	}
}

static void
rdf_shown_select_row_cb (GtkCList *clist,
			 int row,
			 int column,
			 GdkEvent *event,
			 PropertyData *pd)
{
	gtk_widget_set_sensitive (pd->rdf->remove, TRUE);
}

static void
rdf_shown_unselect_row_cb (GtkCList *clist,
			   int row,
			   int column,
			   GdkEvent *event,
			   PropertyData *pd)
{
	if (GTK_CLIST (pd->rdf->shown)->selection == NULL) {
		gtk_widget_set_sensitive (pd->rdf->remove, FALSE);
	}
}

static void
rdf_wipe_trackers_toggled_cb (GtkToggleButton *tb,
			      PropertyData *pd)
{
	pd->summary->preferences->wipe_trackers = gtk_toggle_button_get_active (tb);

	gnome_property_box_changed (pd->box);
}

static void
rdf_add_clicked_cb (GtkButton *button,
		    PropertyData *pd)
{
	GList *p;
	char *url, *text[1];
	int row;

	row = GPOINTER_TO_INT (GTK_CLIST (pd->rdf->all)->selection->data);
	gtk_clist_get_text (GTK_CLIST (pd->rdf->all), row, 0, &url);

	text[0] = url;

	for (p = pd->summary->preferences->rdf_urls; p; p = p->next) {
		if (strcmp (p->data, url) == 0) {
			/* Found it already */
			return;
		}
	}

	pd->summary->preferences->rdf_urls = g_list_prepend (pd->summary->preferences->rdf_urls, g_strdup (url));
	gtk_clist_prepend (GTK_CLIST (pd->rdf->shown), text);

	gnome_property_box_changed (pd->box);
}

static void
rdf_remove_clicked_cb (GtkButton *button,
		       PropertyData *pd)
{
	GList *p;
	int row;

	row = GPOINTER_TO_INT (GTK_CLIST (pd->rdf->shown)->selection->data);
	gtk_clist_remove (GTK_CLIST (pd->rdf->shown), row);

	p = g_list_nth (pd->summary->preferences->rdf_urls, row);
	pd->summary->preferences->rdf_urls = g_list_remove_link (pd->summary->preferences->rdf_urls, p);
	g_free (p->data);
	g_list_free (p);

	gnome_property_box_changed (pd->box);
}

static void
add_dialog_clicked_cb (GnomeDialog *dialog,
		       int button,
		       PropertyData *pd)
{
	char *text[1];

	if (button == 1) {
		return;
	}

	text[0] = gtk_entry_get_text (GTK_ENTRY (pd->new_entry));
	gtk_clist_append (GTK_CLIST (pd->rdf->all), text);
	
	gnome_dialog_close (dialog);
}

static void
rdf_new_url_clicked_cb (GtkButton *button,
			PropertyData *pd)
{
	static GtkWidget *add_dialog = NULL;
	GtkWidget *label;

	if (add_dialog != NULL) {
		gdk_window_raise (add_dialog->window);
		gdk_window_show (add_dialog->window);
		return;
	} 

	add_dialog = gnome_dialog_new (_("Add a new RDF"),
				       GNOME_STOCK_BUTTON_OK,
				       GNOME_STOCK_BUTTON_CANCEL, NULL);
	gtk_signal_connect (GTK_OBJECT (add_dialog), "clicked",
			    GTK_SIGNAL_FUNC (add_dialog_clicked_cb), pd);
	gtk_signal_connect (GTK_OBJECT (add_dialog), "destroy",
			    GTK_SIGNAL_FUNC (gtk_widget_destroyed), &add_dialog);

	label = gtk_label_new (_("Enter the URL of the RDF file you wish to add"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (add_dialog)->vbox), label,
			    TRUE, TRUE, 0);
	pd->new_entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (add_dialog)->vbox), 
			    pd->new_entry, TRUE, TRUE, 0);
	gtk_widget_show_all (add_dialog);
}

static void
rdf_refresh_value_changed_cb (GtkAdjustment *adj,
			      PropertyData *pd)
{
	pd->summary->preferences->rdf_refresh_time = (int) adj->value;
	gnome_property_box_changed (pd->box);
}

static void
rdf_limit_value_changed_cb (GtkAdjustment *adj,
			    PropertyData *pd)
{
	pd->summary->preferences->limit = (int) adj->value;
	gnome_property_box_changed (pd->box);
}

static void
weather_all_select_row_cb (GtkCTree *ctree,
			   GtkCTreeNode *row,
			   int column,
			   PropertyData *pd)
{
	gtk_widget_set_sensitive (pd->weather->add, TRUE);
	pd->weather->selected_node = row;
}

static void
weather_all_unselect_row_cb (GtkCList *clist,
			     GtkCTreeNode *row,
			     int column,
			     PropertyData *pd)
{
	if (clist->selection == NULL) {
		gtk_widget_set_sensitive (pd->weather->add, FALSE);
	}
	pd->weather->selected_node = NULL;
}

static void
weather_shown_select_row_cb (GtkCList *clist,
			     int row,
			     int column,
			     GdkEvent *event,
			     PropertyData *pd)
{
	gtk_widget_set_sensitive (pd->weather->remove, TRUE);
}

static void
weather_shown_unselect_row_cb (GtkCList *clist,
			       int row,
			       int column,
			       GdkEvent *event,
			       PropertyData *pd)
{
	if (clist->selection == NULL) {
		gtk_widget_set_sensitive (pd->weather->remove, FALSE);
	}
}

static void
weather_add_clicked_cb (GtkButton *button,
			PropertyData *pd)
{
	ESummaryWeatherLocation *location;
	GList *p;
	char *text[1];

	location = gtk_ctree_node_get_row_data (GTK_CTREE (pd->weather->all), pd->weather->selected_node);

	for (p = pd->summary->preferences->stations; p; p = p->next) {
		if (strcmp (location->code, p->data) == 0) {
			return; /* Already have it */
		}
	}

	pd->summary->preferences->stations = g_list_prepend (pd->summary->preferences->stations, g_strdup (location->code));
	text[0] = location->name;
	gtk_clist_prepend (GTK_CLIST (pd->weather->shown), text);

	gnome_property_box_changed (pd->box);
}

static void
weather_remove_clicked_cb (GtkButton *button,
			   PropertyData *pd)
{
	int row;
	GList *p;

	row = GPOINTER_TO_INT (GTK_CLIST (pd->weather->shown)->selection->data);
	p = g_list_nth (pd->summary->preferences->stations, row);
	gtk_clist_remove (GTK_CLIST (pd->weather->shown), row);

	pd->summary->preferences->stations = g_list_remove_link (pd->summary->preferences->stations, p);
	g_free (p->data);
	g_list_free (p);

	gnome_property_box_changed (pd->box);
}

static void
weather_refresh_value_changed_cb (GtkAdjustment *adj,
				  PropertyData *pd)
{
	pd->summary->preferences->weather_refresh_time = (int) adj->value;
	gnome_property_box_changed (pd->box);
}

static void
weather_metric_toggled_cb (GtkToggleButton *tb,
			   PropertyData *pd)
{
	if (gtk_toggle_button_get_active (tb) == FALSE) {
		return;
	}

	pd->summary->preferences->units = UNITS_METRIC;
	gnome_property_box_changed (pd->box);
}

static void
weather_imperial_toggled_cb (GtkToggleButton *tb,
			     PropertyData *pd)
{
	if (gtk_toggle_button_get_active (tb) == FALSE) {
		return;
	}

	pd->summary->preferences->units = UNITS_IMPERIAL;
	gnome_property_box_changed (pd->box);
}

static gboolean
make_property_dialog (PropertyData *pd)
{
	struct _MailPage *mail;
	struct _RDFPage *rdf;
	struct _WeatherPage *weather;
	struct _CalendarPage *calendar;

	/* Mail */
	mail = pd->mail = g_new (struct _MailPage, 1);

	/* I think this should be a fancy bonobo thingy */
	mail->all = glade_xml_get_widget (pd->xml, "clist2");
	g_return_val_if_fail (mail->all != NULL, FALSE);

	mail->shown = glade_xml_get_widget (pd->xml, "clist1");
	g_return_val_if_fail (mail->shown != NULL, FALSE);
	fill_mail_shown_clist (GTK_CLIST (mail->shown), pd);
	gtk_signal_connect (GTK_OBJECT (mail->shown), "select-row",
			    GTK_SIGNAL_FUNC (mail_shown_select_row_cb), pd);
	gtk_signal_connect (GTK_OBJECT (mail->shown), "unselect-row",
			    GTK_SIGNAL_FUNC (mail_shown_unselect_row_cb), pd);

	mail->fullpath = glade_xml_get_widget (pd->xml, "checkbutton1");
	g_return_val_if_fail (mail->fullpath != NULL, FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mail->fullpath),
				      pd->summary->preferences->show_full_path);
	gtk_signal_connect (GTK_OBJECT (mail->fullpath), "toggled",
			    GTK_SIGNAL_FUNC (mail_show_full_path_toggled_cb), pd);

	mail->add = glade_xml_get_widget (pd->xml, "button4");
	g_return_val_if_fail (mail->add != NULL, FALSE);
	gtk_signal_connect (GTK_OBJECT (mail->add), "clicked",
			    GTK_SIGNAL_FUNC (mail_add_clicked_cb), pd);

	mail->remove = glade_xml_get_widget (pd->xml, "button5");
	g_return_val_if_fail (mail->remove != NULL, FALSE);
	gtk_signal_connect (GTK_OBJECT (mail->remove), "clicked",
			    GTK_SIGNAL_FUNC (mail_remove_clicked_cb), pd);
	
	/* RDF */
	rdf = pd->rdf = g_new (struct _RDFPage, 1);
	rdf->all = glade_xml_get_widget (pd->xml, "clist6");
	g_return_val_if_fail (rdf->all != NULL, FALSE);
	gtk_signal_connect (GTK_OBJECT (rdf->all), "select-row",
			    GTK_SIGNAL_FUNC (rdf_all_select_row_cb), pd);
	gtk_signal_connect (GTK_OBJECT (rdf->all), "unselect-row",
			    GTK_SIGNAL_FUNC (rdf_all_unselect_row_cb), pd);
	fill_rdf_all_clist (GTK_CLIST (rdf->all));

	rdf->shown = glade_xml_get_widget (pd->xml, "clist5");
	g_return_val_if_fail (rdf->shown != NULL, FALSE);
	gtk_signal_connect (GTK_OBJECT (rdf->shown), "select-row",
			    GTK_SIGNAL_FUNC (rdf_shown_select_row_cb), pd);
	gtk_signal_connect (GTK_OBJECT (rdf->shown), "unselect-row",
			    GTK_SIGNAL_FUNC (rdf_shown_unselect_row_cb), pd);
	fill_rdf_shown_clist (GTK_CLIST (rdf->shown), pd);

	rdf->refresh = glade_xml_get_widget (pd->xml, "spinbutton1");
	g_return_val_if_fail (rdf->refresh != NULL, FALSE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (rdf->refresh),
				   (float) pd->summary->preferences->rdf_refresh_time);
	gtk_signal_connect (GTK_OBJECT (GTK_SPIN_BUTTON (rdf->refresh)->adjustment), "value_changed",
			    GTK_SIGNAL_FUNC (rdf_refresh_value_changed_cb), pd);

	rdf->limit = glade_xml_get_widget (pd->xml, "spinbutton4");
	g_return_val_if_fail (rdf->limit != NULL, FALSE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (rdf->limit), 
				   (float) pd->summary->preferences->limit);
	gtk_signal_connect (GTK_OBJECT (GTK_SPIN_BUTTON (rdf->limit)->adjustment), "value_changed",
			    GTK_SIGNAL_FUNC (rdf_limit_value_changed_cb), pd);

	rdf->wipe_trackers = glade_xml_get_widget (pd->xml, "checkbutton2");
	g_return_val_if_fail (rdf->wipe_trackers != NULL, FALSE);
	gtk_signal_connect (GTK_OBJECT (rdf->wipe_trackers), "toggled",
			    GTK_SIGNAL_FUNC (rdf_wipe_trackers_toggled_cb), pd);
	rdf->add = glade_xml_get_widget (pd->xml, "button9");
	g_return_val_if_fail (rdf->add != NULL, FALSE);

	gtk_widget_set_sensitive (rdf->add, FALSE);
	gtk_signal_connect (GTK_OBJECT (rdf->add), "clicked",
			    GTK_SIGNAL_FUNC (rdf_add_clicked_cb), pd);

	rdf->remove = glade_xml_get_widget (pd->xml, "button10");
	g_return_val_if_fail (rdf->remove != NULL, FALSE);

	gtk_widget_set_sensitive (rdf->remove, FALSE);
	gtk_signal_connect (GTK_OBJECT (rdf->remove), "clicked",
			    GTK_SIGNAL_FUNC (rdf_remove_clicked_cb), pd);
	
	rdf->new_url = glade_xml_get_widget (pd->xml, "button11");
	g_return_val_if_fail (rdf->new_url != NULL, FALSE);
	gtk_signal_connect (GTK_OBJECT (rdf->new_url), "clicked",
			    GTK_SIGNAL_FUNC (rdf_new_url_clicked_cb), pd);

	/* Weather */
	weather = pd->weather = g_new (struct _WeatherPage, 1);
	weather->all = glade_xml_get_widget (pd->xml, "ctree1");
	g_return_val_if_fail (weather->all != NULL, FALSE);
	fill_weather_all_ctree (GTK_CTREE (weather->all));
	gtk_signal_connect (GTK_OBJECT (weather->all), "tree-select-row",
			    GTK_SIGNAL_FUNC (weather_all_select_row_cb), pd);
	gtk_signal_connect (GTK_OBJECT (weather->all), "tree-unselect-row",
			    GTK_SIGNAL_FUNC (weather_all_unselect_row_cb), pd);

	weather->shown = glade_xml_get_widget (pd->xml, "clist3");
	g_return_val_if_fail (weather->shown != NULL, FALSE);
	fill_weather_shown_clist (GTK_CLIST (weather->shown), pd);
	gtk_signal_connect (GTK_OBJECT (weather->shown), "select-row",
			    GTK_SIGNAL_FUNC (weather_shown_select_row_cb), pd);
	gtk_signal_connect (GTK_OBJECT (weather->shown), "unselect-row",
			    GTK_SIGNAL_FUNC (weather_shown_unselect_row_cb), pd);

	weather->refresh = glade_xml_get_widget (pd->xml, "spinbutton5");
	g_return_val_if_fail (weather->refresh != NULL, FALSE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (weather->refresh),
				   (float) pd->summary->preferences->weather_refresh_time);
	gtk_signal_connect (GTK_OBJECT (GTK_SPIN_BUTTON (weather->refresh)->adjustment),
					"value-changed",
					GTK_SIGNAL_FUNC (weather_refresh_value_changed_cb),
					pd);

	weather->metric = glade_xml_get_widget (pd->xml, "radiobutton7");
	g_return_val_if_fail (weather->metric != NULL, FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (weather->metric), (pd->summary->preferences->units == UNITS_METRIC));
	gtk_signal_connect (GTK_OBJECT (weather->metric), "toggled",
			    GTK_SIGNAL_FUNC (weather_metric_toggled_cb), pd);

	weather->imperial = glade_xml_get_widget (pd->xml, "radiobutton8");
	g_return_val_if_fail (weather->imperial != NULL, FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (weather->imperial), (pd->summary->preferences->units == UNITS_IMPERIAL));
	gtk_signal_connect (GTK_OBJECT (weather->imperial), "toggled",
			    GTK_SIGNAL_FUNC (weather_imperial_toggled_cb), pd);

	weather->add = glade_xml_get_widget (pd->xml, "button6");
	g_return_val_if_fail (weather->add != NULL, FALSE);
	gtk_signal_connect (GTK_OBJECT (weather->add), "clicked",
			    GTK_SIGNAL_FUNC (weather_add_clicked_cb), pd);

	weather->remove = glade_xml_get_widget (pd->xml, "button7");
	g_return_val_if_fail (weather->remove != NULL, FALSE);
	gtk_signal_connect (GTK_OBJECT (weather->remove), "clicked",
			    GTK_SIGNAL_FUNC (weather_remove_clicked_cb), pd);

	/* Calendar */
	calendar = pd->calendar = g_new (struct _CalendarPage, 1);
	calendar->one = glade_xml_get_widget (pd->xml, "radiobutton3");
	g_return_val_if_fail (calendar->one != NULL, FALSE);
	calendar->five = glade_xml_get_widget (pd->xml, "radiobutton4");
	g_return_val_if_fail (calendar->five != NULL, FALSE);
	calendar->week = glade_xml_get_widget (pd->xml, "radiobutton5");
	g_return_val_if_fail (calendar->week != NULL, FALSE);
	calendar->month = glade_xml_get_widget (pd->xml, "radiobutton6");
	g_return_val_if_fail (calendar->month != NULL, FALSE);
	calendar->all = glade_xml_get_widget (pd->xml, "radiobutton1");
	g_return_val_if_fail (calendar->all != NULL, FALSE);
	calendar->today = glade_xml_get_widget (pd->xml, "radiobutton2");
	g_return_val_if_fail (calendar->today != NULL, FALSE);

	return TRUE;
}
	
static void
free_property_dialog (PropertyData *pd)
{
	if (pd->rdf) {
		g_free (pd->rdf);
	}
	if (pd->mail) {
		g_free (pd->mail);
	}
	if (pd->weather) {
		g_free (pd->weather);
	}
	if (pd->calendar) {
		g_free (pd->calendar);
	}

	if (pd->xml) {
		gtk_object_unref (GTK_OBJECT (pd->xml));
	}
	if (pd->summary) {
		gtk_object_unref (GTK_OBJECT (pd->summary));
	}

	g_free (pd);
}

static void
property_box_clicked_cb (GnomeDialog *dialog,
			 int page_num,
			 PropertyData *pd)
{
	if (page_num == -1) {
		e_summary_reconfigure (pd->summary);
	}
}

static void
property_box_destroy_cb (GtkObject *object,
			 PropertyData *pd)
{
	if (pd->summary->old_prefs != NULL) {
		e_summary_preferences_free (pd->summary->old_prefs);
		pd->summary->old_prefs = NULL;
	}

	e_summary_preferences_save (pd->summary->preferences);
	free_property_dialog (pd);
}

void
e_summary_configure (GtkWidget *widget,
		     ESummary *summary)
{
	PropertyData *pd;

	if (summary->prefs_window != NULL) {
		gdk_window_raise (summary->prefs_window->window);
		gdk_window_show (summary->prefs_window->window);
		return;
	}

	pd = g_new0 (PropertyData, 1);

	gtk_object_ref (GTK_OBJECT (summary));
	pd->summary = summary;

	if (summary->old_prefs != NULL) {
		e_summary_preferences_free (summary->old_prefs);
	}

	summary->old_prefs = e_summary_preferences_copy (summary->preferences);

	pd->xml = glade_xml_new (EVOLUTION_GLADEDIR "/my-evolution.glade", NULL);
	g_assert (pd->xml != NULL); /* Fixme: Nice GUI to explain what happened */
	pd->box = GNOME_PROPERTY_BOX (glade_xml_get_widget (pd->xml, "dialog1"));
	if (make_property_dialog (pd) == FALSE) {
		g_warning ("Missing some part of XML file");
		free_property_dialog (pd);
		return;
	}

	gtk_signal_connect (GTK_OBJECT (pd->box), "apply",
			    GTK_SIGNAL_FUNC (property_box_clicked_cb), pd);
	gtk_signal_connect (GTK_OBJECT (pd->box), "destroy",
			    GTK_SIGNAL_FUNC (property_box_destroy_cb), pd);
	gtk_widget_show (GTK_WIDGET (pd->box));
}

