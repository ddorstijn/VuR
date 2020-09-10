#include "renderer.h"

int 
main(int argc, char** argv)
{
	vur_init();	

	//while (42) {
		if (!vur_draw()) {
			//break;
		}
	//}

	vur_destroy();
}
