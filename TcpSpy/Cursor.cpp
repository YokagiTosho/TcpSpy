#include "Cursor.hpp"

HCURSOR gArrowCursor;
HCURSOR gWaitCursor;

HCURSOR gCurrentCursor;

void InitCursors() {
	gArrowCursor = LoadCursor(NULL, IDC_ARROW);
	gWaitCursor = LoadCursor(NULL, IDC_WAIT);

	gCurrentCursor = gArrowCursor;
}