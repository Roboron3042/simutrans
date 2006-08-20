/* umgebung.h
 *
 * Hier werden die Kommandozeilenparametr in f�r das Spiel
 * nutzbarer Form gespeichert.
 *
 * von Hansj�rg Malthaner, November 2000
 */

#ifndef dataobj_umgebung_h
#define dataobj_umgebung_h

class cstring_t;

/**
 * Diese Klasse bildet eine Abstraktion der Kommandozeilenparameter.
 * Alle Attribute sind statisch, damit sie �berall zug�nglich sind.
 * Das ist kein Problem, denn sie existieren garantiert nur einmal!
 *
 * @author Hj. Malthaner
 */
class umgebung_t
{
	public:

	/**
	* bei Testl�ufen wird sofort eine standardkarte erzeugt
	*
	* @author Hj. Malthaner
	*/
	static bool testlauf;

	/**
	* im Freispiel gibt es keine Limits wie im echten Spiel, z.B.
	* kann das Konto unbegernzt �berzogen werden
	*
	* @author Hj. Malthaner
	*/
	static bool freeplay;

	/**
	* tag-nacht wechsel zeigen ?
	*
	* @author Hj. Malthaner
	*/
	static bool night_shift;


	/**
	* Stationsabdeckung zeigen
	*
	* @author prissi
	*/
	static bool station_coverage_show;
	static int station_coverage_size;


	/**
	* Namen (St�dte, Haltestellen) anzeigen? (0 .. 3)
	*
	* @author Hj. Malthaner
	*/
	static int show_names;


	/**
	* Welche KIs sollen bei neuen Spielen aktiviert werden?
	*
	* @author V. Meyer
	*/
	static bool automaten[6];


	/**
	* which messages to display where?
	*
	* @author prissi
	*/
	static int message_flags[4];


	/**
	* Wasser (Boden) animieren?
	*
	* @author Hj. Malthaner
	*/
	static bool bodenanimation;


	/**
	* Zuf�llig Fussg�nger in den St�dten erzeugen?
	*
	* @author Hj. Malthaner
	*/
	static bool fussgaenger;


	/**
	* Info-Fenster f�r Fussg�nger und Privatfahrzeuge
	*
	* @author Hj. Malthaner
	*/
	static bool verkehrsteilnehmer_info;

	/* How many tiles can a simutrans car go, before it forever break ...
	* @author prissi
	*/
	static long stadtauto_duration;

	/**
	* Info-Fenster f�r B�ume
	* @author prissi
	*/
	static bool tree_info;



	/**
	* Info-Fenster f�r Townhall
	* @author prissi
	*/
	static bool townhall_info;

	/**
	* Only one info window
	* @author prissi
	*/
	static bool single_info;

	/**
	* Only one info window
	* @author prissi
	*/
	static bool window_buttons_right;

	/**
	* Produce more debug info ?
	*
	* @author Hj. Malthaner
	*/
	static bool verbose_debug;


	/**
	* Startkapital f�r Spieler
	*
	* @author Hj. Malthaner
	*/
	static int starting_money;


	/**
	* Wartungskosten f�r Geb�ude
	*
	* @author Hj. Malthaner
	*/
	static int maint_building;


	/**
	* Wartungskosten f�r Wege
	*
	* @author Hj. Malthaner
	*/
	static int maint_way;


	/**
	* Wartungskosten f�r Oberleitungen
	*
	* @author Hj. Malthaner
	*/
	static int maint_overhead;


	/**
	* Use numbering for stations?
	*
	* @author Hj. Malthaner
	*/
	static bool numbered_stations;

	/**
	* show month in date?
	*
	* @author hsiegeln
	*/
	static bool show_month;


	/**
	* Max. L�nge f�r initiale Stadtverbindungen
	*
	* @author Hj. Malthaner
	*/
	static int intercity_road_length;


	/**
	* Typ (Name) initiale Stadtverbindungen
	*
	* @author Hj. Malthaner
	*/
	static cstring_t * intercity_road_type;

	/**
	 * Typ (Name) initiale Stadtstrassen
	 *
	 * @author Hj. Malthaner
	 */
	static cstring_t * city_road_type;

	/**
	* Should the timeline be activated?
	*
	* @author Hj. Malthaner
	*/
	static bool use_timeline;


	/**
	* Starting year of the game
	*
	* @author Hj. Malthaner
	*/
	static int starting_year;


	/* prissi: maximum number of steps for breath search */
	static int max_route_steps;


	/* prissi: maximum number of steps for breath search */
	static int max_transfers;


	/* prissi: do autosave every month? */
	static int autosave;

	/* prissi: crossconnect all factories (like OTTD and similar games) */
	static bool crossconnect_factories;

	/* prissi: drive on the left side of the road */
	static bool drive_on_left;
};

#endif
