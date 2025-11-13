
#include "fat.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

char sector_buf[512];
char rde_region[16384];
int fd = 0;

int read_sector_from_disk_image(unsigned int sector_num, char *buf, unsigned int nsectors) {

	// position the OS index
	lseek(fd, sector_num * 512, SEEK_SET);

	// Read one sector from disk image
	int n = read(fd, buf, 512 * nsectors);

}

int main() {
	struct boot_sector *bs = sector_buf;
	fd = open("disk.img", O_RDONLY);
	read_sector_from_disk_image(0, sector_buf);

	printf("sectors per cluseter = %d\n", bs->num_sectors_per_cluster);
	printf("reserved sectors = %d\n", bs->num_reserved_sectors);
	printf("num fat tables = %d\n", bs->num_fat_tables);
	printf("num RDEs = %d\n", bs->num_root_dir_entries);


	// Read RDE Region
	read_sector_from_disk_image( 0 + bs->num_reserved_sectors + bs->num_fat_tables * bs->num_sectors_per_fat,
			rde_region, // buffer to hold the RDE table
			32); // Number of sectors in the RDE table

	return 0;
}


