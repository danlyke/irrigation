/* simple.c
   Simple libftdi usage example
   This program is distributed under the GPL, version 2
*/

#include <stdio.h>
#include <stdlib.h>
#include <ftdi.h>

int main(int argc, char **argv)
{
    int ret;
    struct ftdi_context *ftdi;
//    struct ftdi_version_info version;
    if ((ftdi = ftdi_new()) == 0)
    {
        fprintf(stderr, "ftdi_new failed\n");
        return EXIT_FAILURE;
    }
//    version = ftdi_get_library_version();
//    printf("Initialized libftdi %s (major: %d, minor: %d, micro: %d, snapshot ver: %s)\n",
//           version.version_str, version.major, version.minor, version.micro,
//           version.snapshot_str);
    if ((ret = ftdi_usb_open(ftdi, 0x0403, 0x6003)) < 0)
    {
        fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }
// Read out FTDIChip-ID of R type chips
    if (ftdi->type == TYPE_R)
    {
        unsigned int chipid;
        printf("ftdi_read_chipid: %d\n", ftdi_read_chipid(ftdi, &chipid));
        printf("FTDI chipid: %X\n", chipid);
    }

    if (argc > 1)
    {
        ftdi_set_bitmode(ftdi, 0xFF, BITMODE_BITBANG);
        for (int i = 1; i < argc; ++i)
        {
            if (i != 1) sleep(1);
            int bit = atoi(argv[i]);
            unsigned char buf[1] = {0x0};
            if (bit == 0)
            {
                printf("Turning off relays (%x)\n", buf[0]);
            }
            else
            {
                --bit;
                buf[0] ^= (1 << bit);
                printf("Turning on relay %d (%x)\n", bit, buf[0]);
            }

            ftdi_write_data(ftdi, buf, 1);
        }
    }
    


    if ((ret = ftdi_usb_close(ftdi)) < 0)
    {
        fprintf(stderr, "unable to close ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }
    ftdi_free(ftdi);
    return EXIT_SUCCESS;
}
