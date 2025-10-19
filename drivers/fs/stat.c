#include "fs/stat.h"

int S_ISDIR(uint32_t mode){
	return (mode == S_IFDIR ? 1 : 0);
}
