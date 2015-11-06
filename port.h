#ifndef __PORT_H
#define __PORT_H

#ifndef WIN32
	#ifndef MAX_PATH
		#define MAX_PATH 2048  /* i don't known the corrispondent of MAX_PATH in linux */
	#endif
#else
	#define snprintf _snprintf
#endif

#endif
