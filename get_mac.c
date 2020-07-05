#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>


#define HCIGETCONNINFO	_IOR('H', 213, int)
//gcc -o get_mac get_mac.c -lbluetooth

int read_oui_file(char* lap, bdaddr_t* bdaddrs, char* oui_name)
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    char delimiter[2] = {':','\0'};
    char* pdelimiter = &delimiter[0];
    int count = 0;
    //printf("\n%s\n", lap);
   
    fp = fopen(oui_name, "r");
    if (fp == NULL){
        
        printf("Bad OUI file!\n");
        exit(EXIT_FAILURE);
    }


    char MAC[50];   
    char* pMAC = &MAC[0];

    while ((read = getline(&line, &len, fp)) != -1) {
        //printf("Retrieved line of length %zu:\n", read);
        strcpy(pMAC,  line);
        pMAC[strlen(pMAC) - 1] = '\0';    
        strcat(pMAC, pdelimiter);
        strcat(pMAC, lap);
        str2ba(pMAC, bdaddrs++);
        count++;
        
    }

    fclose(fp);
    if (line)
        free(line);
    return count;
}

int main(int argc, char **argv)
{
    struct hci_conn_info_req *cr; 
    bdaddr_t bdaddr, bdaddrs[10000];
	int8_t rssi;  
    uint16_t handle;
	uint8_t role;
    bool ping_sta = false;
	unsigned int ptype, counter, count = 0;
    int dev_id, dd, opt;
    char strAddr[50], btaddr[50];  
    char *pbtaddr = &btaddr[0];
    char *oui_fname = NULL, *dev_name, *plap = NULL;
   
  
    while((opt = getopt(argc, argv, "d:l:")) != -1)  
    {  
        switch(opt)  
        {  
            case 'd': 
                printf("dictionary filename: %s\n", optarg);
                oui_fname = optarg;
                break; 

            case 'l':  
                printf("LAP address %s\n", optarg); 
                plap = optarg; 
                break;   

            case '?':  
                printf("unknown option: %c\n", optopt); 
                break;  
        }  
    }     


    if((oui_fname != NULL) && (plap != NULL)){

        count = read_oui_file(plap, &bdaddrs[0], oui_fname); 

    } else {

        printf("\nargs fail. try -d <oui dictinary file> -l <lap 11:22:33>\n");
        
    }

    for(counter = 0 ; counter < count ; counter++)
    {
        ba2str( &bdaddrs[counter], &strAddr[0]);
        printf("Pinging: %s...", strAddr);
        fflush( stdout );
        //int dev_id = hci_devid( "01:23:45:67:89:AB" ); Manually select device instead of hci_get_route
        dev_id = hci_get_route(&bdaddrs[counter]);
        if (dev_id < 0) {

            perror("\nDevice is not available.\n");
            exit(1);
        }        
     
        dd = hci_open_dev(dev_id);
        if (dd < 0) {
            perror("\nHCI device open failed\n");
            exit(1);
        }
        role = 1;
        ptype = HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5;
        if (hci_create_connection(dd, &bdaddrs[counter], htobs(ptype), htobs(0x0000), role, &handle, 25000) < 0) {

            printf("no reply.\n");
            hci_close_dev(dd);
            continue;
           

        }

        cr = malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
        if (!cr) {
            perror("\nCan't allocate memory\n");
            hci_disconnect(dd, cr->conn_info->handle, 0x13, 100);
            hci_close_dev(dd);
            exit(1);
        }

        bacpy(&cr->bdaddr, &bdaddrs[counter]);
        cr->type = ACL_LINK;
        if (ioctl(dd, HCIGETCONNINFO, (unsigned long) cr) < 0) {
            perror("\nGet connection info failed\n");           
            hci_disconnect(dd, cr->conn_info->handle, 0x13, 100);
            free(cr);
            hci_close_dev(dd);
            continue;
        }

        if (hci_read_rssi(dd, htobs(cr->conn_info->handle), &rssi, 1000) < 0) {
            perror("\nRead RSSI failed\n");
            hci_disconnect(dd, cr->conn_info->handle, 0x13, 100);
            free(cr);
            hci_close_dev(dd);
            continue;
        }
       
        hci_disconnect(dd, cr->conn_info->handle, 0x13, 100);
        hci_close_dev(dd);
        free(cr);
        printf("reply RSSI: %d\n", rssi);
        

        //break;
    }
    
    
    return 0;
}
