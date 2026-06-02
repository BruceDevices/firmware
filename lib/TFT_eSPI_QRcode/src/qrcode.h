#include <display/tft.h>

class QRcode
{
	private:
		tft_display *tft;
		void render(int x, int y, int color);

	public:
		QRcode(tft_display *display);
		void init();
		void create(String message);	
};
