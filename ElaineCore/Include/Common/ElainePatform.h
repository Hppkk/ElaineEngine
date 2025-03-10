#pragma once


#define ELAINE_PLATFORM_WINDOWS		1
#define ELAINE_PLATFORM_APPLE		2
#define ELAINE_PLATFORM_ANDROID		3
#define ELAINE_PLATFORM_LINUX		4

#define ELAINE_RENDER_API_DX12		11
#define ELAINE_RENDER_API_DX11		12
#define ELAINE_RENDER_API_VULKAN	13
#define ELAINE_RENDER_API_METAL		14
#define ELAINE_RENDER_API_OPENGL	15




#if defined (_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__NT__)
	#define ELAINE_PLATFORM ELAINE_PLATFORM_WINDOWS
#elif __APPLE__
	#define ELAINE_PLATFORM ELAINE_PLATFORM_APPLE
#elif __ANDROID__
	#define ELAINE_PLATFORM ELAINE_PLATFORM_ANDROID
#elif __linux__
	#define ELAINE_PLATFORM ELAINE_PLATFORM_LINUX
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__)
	#define ELAINE_RENDER_API ELAINE_RENDER_API_DX12
#elif __APPLE__
	#define ELAINE_RENDER_API ELAINE_RENDER_API_METAL
#else
	#define ELAINE_RENDER_API ELAINE_RENDER_API_VULKAN
#endif

typedef unsigned char		uint8;
typedef unsigned short int	uint16;
typedef unsigned int		uint32;
typedef unsigned long long	uint64;
typedef	signed char			int8;
typedef signed short int	int16;
typedef signed int	 		int32;
typedef signed long long	int64;
