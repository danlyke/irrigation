#include <stdlib.h>
#include "documents.h"

DocumentsList documents[] = 
{
	{
		"/", 
		"text/html",
		"<html><head><title>GVS Indexer Controller</title></head><body>\n<h1>GVS Indexer Controller</h1>\n"
		"<p>For testing you can try</p>\n"
		"<ul>\n"
		"<li><a href=\"rotate.cgi?0\">Rotate to 0 degrees</a>\n"
		"<li><a href=\"rotate.cgi?90\">Rotate to 90 degrees</a>\n"
		"<li><a href=\"rotate.cgi?180\">Rotate to 180 degrees</a>\n"
		"<li><a href=\"rotate.cgi?270\">Rotate to 270 degrees</a>\n"
		"<li><a href=\"unlock.cgi\">Unlock the rig</a>\n"
		"<li><a href=\"test.cgi\">A few diagnostics</a>\n"
		"</ul>\n"
		"<p>This version of the controller code was compiled on " __DATE__ "</p>\n"
		"</body></html>\n"
	},
	{ NULL, NULL, NULL }
};
