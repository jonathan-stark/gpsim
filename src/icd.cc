// FIXME. Stolen from Geir and extended with debug stuff.


#include <stdio.h>
#include <iostream.h>
#include <iomanip.h>
#include <string>
#include <list>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>

#include "../config.h"
#include "pic-processor.h"

#include "icd.h"

#define BAUDRATE B57600

#ifdef HAVE_GUI
extern void gui_refresh(void);
extern int use_gui;
#endif

int use_icd;
int bulk_flag;

static int icd_fd;  /* file descriptor for serial port */

static int icd_sync(void);

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
class icd_Register : public file_register
{
public:
    file_register *replaced;
    int is_stale;

    icd_Register();

    virtual REGISTER_TYPES isa(void) {return replaced->isa();};
    virtual char *name(void) {return replaced->name();};

    virtual void put_value(unsigned int new_value);
    virtual void put(unsigned int new_value);
    virtual unsigned int get_value(void);
    virtual unsigned int get(void);
};

class icd_StatusReg : public Status_register
{
public:
    Status_register *replaced;
    int is_stale;

    icd_StatusReg();

    virtual REGISTER_TYPES isa(void) {return replaced->isa();};
    virtual char *name(void) {return replaced->name();};

    virtual void put_value(unsigned int new_value);
    virtual void put(unsigned int new_value);
    virtual unsigned int get_value(void);
    virtual unsigned int get(void);
};

class icd_WREG : public WREG
{
public:
    WREG *replaced;  
    int is_stale;

    icd_WREG();

    virtual REGISTER_TYPES isa(void) {return replaced->isa();};
    virtual char *name(void) {return replaced->name();};

    virtual void put_value(unsigned int new_value);
    virtual void put(unsigned int new_value);
    virtual unsigned int get_value(void);
    virtual unsigned int get(void);
};

class icd_PCLATH : public PCLATH
{
public:
    PCLATH *replaced;
    int is_stale;

    icd_PCLATH();

    virtual REGISTER_TYPES isa(void) {return replaced->isa();};
    virtual char *name(void) {return replaced->name();};

    virtual void put_value(unsigned int new_value);
    virtual void put(unsigned int new_value);
    virtual unsigned int get_value(void);
    virtual unsigned int get(void);
};
class icd_FSR : public FSR
{
public:
    FSR *replaced;   
    int is_stale;

    icd_FSR();

    virtual REGISTER_TYPES isa(void) {return replaced->isa();};
    virtual char *name(void) {return replaced->name();};

    virtual void put_value(unsigned int new_value);
    virtual void put(unsigned int new_value);
    virtual unsigned int get_value(void);
    virtual unsigned int get(void);
};

class icd_PC : public Program_Counter
{
public:
    Program_Counter *replaced;
    int is_stale;

    icd_PC();

    virtual void put_value(unsigned int new_value);
    virtual unsigned int get_value(void);
};

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

static void udelay(unsigned usec)
{
    /* wait for msec milliseconds or more ... */

    struct timespec time;

    time.tv_sec  = usec / 1000000;
    time.tv_nsec = ( usec % 1000000) * 1000;

    nanosleep(&time,NULL);

}

static void dtr_set()
{
    int flag = TIOCM_DTR;

    if(icd_fd<0) return;

    if(ioctl(icd_fd, TIOCMBIS, &flag)) {
	perror("ioctl");
	exit(-1);
    }
}

static void dtr_clear()
{
    int flag  = TIOCM_DTR;

    if(icd_fd<0) return;

    if(ioctl(icd_fd, TIOCMBIC, &flag)) {
	perror("ioctl");
	exit(-1);
    }
}

static void rts_set()
{
    int flag = TIOCM_RTS;

    if(icd_fd<0) return;

    if(ioctl(icd_fd, TIOCMBIS, &flag)) {
	perror("ioctl");
	exit(-1);
    }
}

static void rts_clear()
{
    int flag  = TIOCM_RTS;

    if(icd_fd<0) return;

    if(ioctl(icd_fd, TIOCMBIC, &flag)) {
	perror("ioctl");
	exit(-1);
    }
}

void icd_hw_reset()
{
    if(icd_fd<0) return;

    rts_clear();
    dtr_clear();   /* reset */
    udelay(10000);
    dtr_set();     /* remove reset */
}

static int icd_write(const char *s)
{
    if(icd_fd<0) return -1;

    cout << "Command: "<<s<<endl;

    write(icd_fd,s,  strlen(s));   /* Error checking ... */
}

static int icd_read(unsigned char *p, int len)
{
    int n_read;

    n_read=read(icd_fd,p,1);

    rts_clear();
    udelay(1);
    rts_set();

    if(n_read != 1) {
	cout << "Error in number of bytes read \n";
	cout << "len="<<len<<endl;
	return 0;
    }

    if(len>1)
	return n_read+icd_read(p+1,len-1);

    return n_read;
}

#define MAX_CMD_LEN 100

static int icd_cmd(const char *cmd, ...)
{
    char command[MAX_CMD_LEN];
    unsigned char resp[3];
    va_list ap;

    if(icd_fd<0) return -1;

    va_start(ap, cmd);
    (void) vsnprintf(command,MAX_CMD_LEN, cmd, ap);
    va_end(ap);

    icd_write(command);

    if(!icd_read(resp,2))
    {
	icd_sync();
	icd_write(command);
	if(!icd_read(resp,2))
	{
	    cout << "Command "<<command<<" failed"<<endl;
	    return -1;
	}
    }

    return (resp[0] << 8 ) |  resp[1];
}

static int icd_sync(void)
{
    // This doesn't work FIXME.

    int tries=3;
    unsigned char buf[0x42];

    while(tries>0)
    {
	tries--;

	if(icd_cmd("$$6307\r")==1)
	    return 1;
	icd_write("$");
	icd_read(buf,0x42);
    }

    puts("***************** DID NOT SYNC!");

    return 0;
}

static int icd_baudrate_init()
{
    int tries=3;
    char ch;

    if(icd_fd<0) return 0;

    while(tries) {
	write(icd_fd,"U",1);

	if(read(icd_fd,&ch,1) > 0) {
	    rts_clear();
	    udelay(10);
	    rts_set();
	    if(ch=='u') {
		return 1;
	    }
	}
	tries--;
    }

    return 0;
}

char *icd_target(void)
{
    static char return_string[256];
    unsigned int dev_id, type,rev;

    if(icd_fd<0) return NULL;

    dev_id=icd_cmd("$$7020\r");
    type = (dev_id>>5) & 0x1FF;
    rev = type & 0x1F;

    if(dev_id == 0x3FFF) {
	sprintf(return_string,"no target");
    } else {
	switch(type) {
	case 0x68:
	    sprintf(return_string,"16F870 rev %d",rev);
	    break;
	case 0x69:
	    sprintf(return_string,"16F871 rev %d",rev);
	    break;
	case 0x47:
	    sprintf(return_string,"16F872 rev %d",rev);
	    break;
	case 0x4B:
	    sprintf(return_string,"16F873 rev %d",rev);
	    break;
	case 0x49:
	    sprintf(return_string,"16F874 rev %d",rev);
	    break;
	case 0x4F:
	    sprintf(return_string,"16F876 rev %d",rev);
	    break;
	case 0x4D:
	    sprintf(return_string,"16F877 rev %d",rev);
	    break;

	default:
	    sprintf(return_string,"Unknown, device id = %02X",dev_id);
	    break;
	}
    }
    return return_string;
}

struct termios oldtio, newtio;

void put_dumb_register(file_register **frp, int address)
{
    file_register *fr = *frp;
    icd_Register *ir = new icd_Register;
    ir->cpu = fr->cpu;
    *frp = ir;
    ir->replaced = fr;
    ir->address = address;
    ir->xref = fr->xref;
}
void put_dumb_status_register(Status_register **frp)
{
    Status_register *fr = *frp;
    icd_StatusReg *ir = new icd_StatusReg;
    ir->cpu = fr->cpu;
    *frp = ir;
    ir->replaced = fr;
    ir->address = fr->address;
    ir->xref = fr->xref;
}
void put_dumb_pc_register(Program_Counter **frp)
{
    Program_Counter *fr = *frp;
    icd_PC *ir = new icd_PC;
    ir->cpu = fr->cpu;
    ir->memory_size_mask = fr->memory_size_mask;
    *frp = ir;
    ir->replaced = fr;
    ir->xref = fr->xref;
}
void put_dumb_pclath_register(PCLATH **frp)
{
    PCLATH *fr = *frp;
    icd_PCLATH *ir = new icd_PCLATH;
    ir->cpu = fr->cpu;
    *frp = ir;
    ir->replaced = fr;
    ir->xref = fr->xref;
}
void put_dumb_w_register(WREG **frp)
{
    WREG *fr = *frp;
    icd_WREG *ir = new icd_WREG;
    ir->cpu = fr->cpu;
    *frp = ir;
    ir->replaced = fr;
    ir->xref = fr->xref;
}
void put_dumb_fsr_register(FSR **frp)
{
    FSR *fr = *frp;
    icd_FSR *ir = new icd_FSR;
    ir->cpu = fr->cpu;
    *frp = ir;
    ir->replaced = fr;
    ir->xref = fr->xref;
}

static void create_dumb_register_file(void)
{
    pic_processor *cpu=get_processor(1);
    for(int i=0;i<cpu->register_memory_size();i++)
    {
	put_dumb_register(&cpu->registers[i], i);
    }
    put_dumb_status_register(&cpu->status);
    put_dumb_pc_register(&cpu->pc);
    put_dumb_pclath_register(&cpu->pclath);
    put_dumb_w_register(&cpu->W);
    put_dumb_fsr_register(&cpu->fsr);
}


int icd_connect(char *port)
{
    pic_processor *pic=get_processor(1);

    if(pic==NULL)
    {
	cout << "You have to load the .cod file (or .hex and processor)" << endl;
	return 0;
    }

    if((icd_fd=open(port, O_NOCTTY | O_RDWR | O_SYNC)) == -1) {
	perror("Error opening device:");
	return 0;
    }

    tcgetattr(icd_fd, &oldtio);

    memset(&newtio,0, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag =  0;
    newtio.c_lflag =  0;

    newtio.c_cc[VTIME] = 100;
    newtio.c_cc[VMIN] = 0;

    tcflush(icd_fd, TCIFLUSH);
    tcsetattr(icd_fd, TCSANOW, &newtio);

    icd_hw_reset();

    rts_set();

    if(!icd_baudrate_init()) {
	fprintf(stderr,"Can't initialize the ICD\n");
    }

    create_dumb_register_file();

    use_icd=1;

    icd_cmd("$$6300\r");
    /* I really don't know what this is, but MPLAB does
     this. The program works ok without this though ..*/


    //	printf("ICD ver %s, target %s\n",icd_version(),icd_target());
    //	printf("Vdd %.2f, Vpp %.2f\n", icd_vdd(), icd_vpp());

    if(icd_has_debug_module())
    {
	if(verbose)
	    cout << "Debug module present"<<endl;
    }
    else
    {
	cout << "Debug module not present. Enabling..."<<flush;
	icd_cmd("$$7008\r"); // Enable debug module
	cout << "Done." << endl;
    }

    icd_reset();

    pic->pc->get_value(); // FIXME
    gi.simulation_has_stopped();

    return 1;
}

int icd_has_debug_module(void)
{
    if(icd_fd<0) return 0;

    icd_cmd("$$700A\r");
    if(icd_cmd("$$6307\r")==1)
	return 1;
    return 0;
}

// Call when gpsim exits.
int icd_disconnect(void)
{
    if(icd_fd<0) return 0;

    cout << "ICD disconnect" << endl;
    icd_hw_reset();
    close(icd_fd);

    return 1;
}

int icd_reset(void)
{
    if(icd_fd<0) return 0;

    cout << "Reset" << endl;
    icd_cmd("$$700A\r");
    icd_cmd("$$701B\r");

    return 1;
}

int icd_detected(void)
{
    if(icd_fd<0) return 0;

    if(use_icd)
	return 1;
    return 0;
}

char *icd_version(void)
{
    static char ret[256];
    unsigned int ver1,ver2;

    if(icd_fd<0) return NULL;

    ver1 = icd_cmd("$$7F00\r");
    ver2 = icd_cmd("$$7021\r");

    sprintf(ret, "%X.%02X.%02X", ver1>>8, ver1&0xFF, ver2);
    return ret;
}

float icd_vdd(void)
{
    unsigned int vdd=0;

    if(icd_fd<0) return 0.0;

    vdd=icd_cmd("$$701C\r");

    return ((double)vdd) / 40.0;
}

float icd_vpp(void)
{
    unsigned int vpp=0;

    if(icd_fd<0) return 0.0;

    icd_cmd("$$7000\r");     // enable Vpp
    vpp=icd_cmd("$$701D\r")  & 0xFF;  // What the heck does the high byte contain ?
    icd_cmd("$$7001\r");     // disable Vpp

    return ((double)vpp) / 11.25;
}

static void make_stale(void)
{
    if(icd_fd<0) return;

    pic_processor *cpu=get_processor(1);
    for(int i=0;i<cpu->register_memory_size();i++)
    {
	icd_Register *ir = dynamic_cast<icd_Register*>(cpu->registers[i]);
	assert(ir!=NULL);
	ir->is_stale=1;
    }

    icd_WREG *iw = dynamic_cast<icd_WREG*>(cpu->W);
    assert(iw!=NULL);
    iw->is_stale=1;

    icd_PC *ipc = dynamic_cast<icd_PC*>(cpu->pc);
    assert(ipc!=NULL);
    ipc->is_stale=1;

    icd_PCLATH *ipclath = dynamic_cast<icd_PCLATH*>(cpu->pclath);
    assert(ipclath!=NULL);
    ipclath->is_stale=1;

    icd_FSR *ifsr = dynamic_cast<icd_FSR*>(cpu->fsr);
    assert(ifsr!=NULL);
    ifsr->is_stale=1;

    icd_StatusReg *isreg = dynamic_cast<icd_StatusReg*>(cpu->status);
    assert(isreg!=NULL);
    isreg->is_stale=1;
}

int icd_step(void)
{
    if(icd_fd<0) return 0;

    make_stale();

    icd_cmd("$$700E\r");
    return 1;
}

int icd_run(void)
{
    if(icd_fd<0) return 0;

    make_stale();

    if(icd_cmd("$$700F\r")!=1)
    {
	icd_sync();
	if(icd_cmd("$$700F\r")!=1)
	    cout << "fjsdk" << endl;
    }
    return 1;
}

int icd_stopped(void)
{
    if(icd_fd<0) return 0;

    if(icd_cmd("$$701E\r")!=1)
	return 1;
    return 0;
}

int icd_halt(void)
{
    if(icd_fd<0) return 0;

    make_stale();
    icd_cmd("$$700D\r");
    return 1;
}

int icd_set_break(int address)
{
    if(icd_fd<0) return 0;

    cout << "Set breakpoint on address " << address << endl;
    return 1;
}

int icd_clear_break(void)
{
    if(icd_fd<0) return 0;

    cout << "Clear breakpoints" << endl;
    return 1;
}

void icd_set_bulk(int flag)
{
    bulk_flag=flag;
}

// Get the value of the specified file memory address
/*int icd_read_file(int address)
{
	unsigned char buf[8];
	int offset = address - address%8;
        if(icd_fd<0) return 0;

	int value;

	cout << "this is deprecated" << endl;
	
	icd_cmd("$$%04X\r",0x7800+offset);
	icd_cmd("$$7C08\r");

	icd_write("$$7D08\r");
	icd_read(buf,8);
	
	value = buf[address%8];
	
	pic_processor *pic=get_processor(1);

		    //if(gpsim_register_is_valid(1,REGISTER_RAM,address) &&
		    //   !gpsim_register_is_alias(1,REGISTER_RAM,address))
		    {
			switch(address)
			{
			case 2:
			case 3:
			case 4:
			case 10:
                            break;
			default:
			    pic->registers[address]->put_value(value);
			cout << "Read file address " << address << "=" << value << endl;
                            break;
			}
		    }
	
	return 1;
}

int icd_write_file(int address, int data)
{
        if(icd_fd<0) return 0;

	

	printf("Write file address 0x%04X with data 0x%02X\n",address,data);
	return 1;
}

int icd_read_eeprom(int address)
{
        if(icd_fd<0) return 0;

	

	printf("Read eeprom address 0x%04X\n",address);
	return 1;
}

int write_eeprom(int address, int data)
{
        if(icd_fd<0) return 0;

	

	printf("Write eeprom address 0x%04X with data 0x%02X\n",address,data);
	return 1;
}

int icd_get_state()
{
    int pc=4, status, w, pclath, fsr;

    if(icd_fd<0) return 0;

	

	cout << "Get state" << endl;
	pc=icd_cmd("$$701F\r");
	status=icd_cmd("$$7016\r")&0x00ff;
	w=icd_cmd("$$7017\r")&0x00ff;
	pclath=icd_cmd("$$7018\r")&0x00ff;
	fsr=icd_cmd("$$7019\r")&0x00ff;

	pic_processor *pic = get_processor(1);
	pic->pc->put_value(pc);
	pic->status->put_value(status);
	pic->W->put_value(w);
	pic->pclath->put_value(pclath);
        pic->fsr->put_value(fsr);

	return 1;
}
 */
// Get all of file memory
/*int icd_get_file()
{
	unsigned char buf[64];
	
        if(icd_fd<0) return 0;

	

	cout << "Get file" << endl;

	pic_processor *pic=get_processor(1);
	
	for(int i=0;i<pic->register_memory_size()/0x40;i++)
	{
	    if(icd_cmd("$$%04X\r",0x7A00+i)!=i)
		puts("EEEEEEEEEEEEEEEEEEEEE");

		icd_write("$$7D40\r");
		icd_read(buf,64);
		for(int j=0;j<64;j++)
		{
		    if(gpsim_register_is_valid(1,REGISTER_RAM,i*0x40+j) &&
		       !gpsim_register_is_alias(1,REGISTER_RAM,i*0x40+j))
		    {
			switch(i*0x40+j)
			{
			case 2:
			case 3:
			case 4:
			case 10:
                            break;
			default:
			    pic->registers[i*0x40+j]->put_value(buf[j]);
                            break;
			}
		    }
		}
//	}
	
	return 1;
}
*/

icd_Register::icd_Register()
{
    replaced=0;
    value=0x42;
    is_stale=1;
};

void icd_Register::put_value(unsigned int new_value)
{
}

void icd_Register::put(unsigned int new_value)
{
}

unsigned int icd_Register::get_value(void)
{
    return(get());
}

unsigned int icd_Register::get(void)
{
    if(is_stale)
    {
	switch(address)
	{
	case 2:
	    value = icd_cmd("$$701F\r");
	    cpu->pcl->value = value & 0xff;
	    cpu->pclath->value = value >> 8;

	    is_stale=0;
	    break;
	case 3:
	    value = icd_cmd("$$7016\r")&0x00ff;
	    is_stale=0;
	    if(xref)
		xref->update();
	    break;
	case 4:
	    value = icd_cmd("$$7019\r")&0x00ff;
	    is_stale=0;
	    if(xref)
		xref->update();
	    break;
	case 10:
	    value = icd_cmd("$$701F\r");
	    cpu->pcl->value = value & 0xff;
	    cpu->pclath->value = value >> 8;

	    is_stale=0;
	    break;
	default:
	    {
		if(bulk_flag==0)
		{
		    unsigned char buf[8];
		    int offset = address - address%8;
		    icd_cmd("$$%04X\r",0x7800+offset);
		    icd_cmd("$$7C08\r");
		    icd_write("$$7D08\r");
		    icd_read(buf,8);
		    for(int i=0;i<8;i++)
		    {
			switch(offset+i)
			{
			case 2:
			case 3:
			case 4:
			case 10:
			    break;
			default:
			    icd_Register *ifr = static_cast<icd_Register*>(cpu->registers[offset+i]);
			    assert(ifr!=NULL);
			    ifr->value=buf[i];
			    ifr->is_stale=0;
			    break;

			}
		    }
		    for(int i=0;i<0x8;i++)
		    {
			switch(offset+i)
			{
			case 2:
			case 3:
			case 4:
			case 10:
			    break;
			default:
			    icd_Register *ifr = static_cast<icd_Register*>(cpu->registers[offset+i]);
			    assert(ifr!=NULL);
			    if(ifr->xref)
				ifr->xref->update();
			    break;
			}
		    }
		}
		else
		{
		    unsigned char buf[64];
		    int offset=address-address%0x40;
		    assert(offset>=0);
		    if(icd_cmd("$$%04X\r",0x7A00+offset/0x40)!=offset/0x40)
			puts("DDDDDDDDDDDDDDDDDDD");
		    icd_write("$$7D40\r");
		    int n_read = icd_read(buf,0x40);
		    for(int i=0;i<0x40;i++)
		    {
			switch(offset+i)
			{
			case 2:
			case 3:
			case 4:
			case 10:
			    break;
			default:
			    icd_Register *ifr = static_cast<icd_Register*>(cpu->registers[offset+i]);
			    assert(ifr!=NULL);
			    ifr->value=buf[i];
			    ifr->is_stale=0;
			    break;

			}
		    }
		    for(int i=0;i<0x40;i++)
		    {
			switch(offset+i)
			{
			case 2:
			case 3:
			case 4:
			case 10:
			    break;
			default:
			    icd_Register *ifr = static_cast<icd_Register*>(cpu->registers[offset+i]);
			    assert(ifr!=NULL);
			    if(ifr->xref)
				ifr->xref->update();
			    break;
			}
		    }
		}
	    }
	    break;
	}
    }
    return(value);
}

icd_WREG::icd_WREG()
{
    replaced=0;
    value=0x42;
    is_stale=1;
};

void icd_WREG::put_value(unsigned int new_value)
{
}

void icd_WREG::put(unsigned int new_value)
{
}

unsigned int icd_WREG::get_value(void)
{
    return(get());
}

unsigned int icd_WREG::get(void)
{
    if(is_stale)
    {
	value = icd_cmd("$$7017\r")&0x00ff;
	is_stale=0;
	if(xref)
	    xref->update();
    }
    return(value);
}

icd_StatusReg::icd_StatusReg()
{
    replaced=0;
    value=0x42;
    is_stale=1;
};

void icd_StatusReg::put_value(unsigned int new_value)
{
}

void icd_StatusReg::put(unsigned int new_value)
{
}

unsigned int icd_StatusReg::get_value(void)
{
    if(icd_fd<0) return 0;

    return(get());
}

unsigned int icd_StatusReg::get(void)
{
    if(is_stale)
    {
	value = icd_cmd("$$7016\r")&0x00ff;
	is_stale=0;
	if(xref)
	    xref->update();
    }
    return(value);
}


icd_FSR::icd_FSR()
{
    replaced=0;
    value=0x42;
    is_stale=1;
};

void icd_FSR::put(unsigned int new_value)
{
}
void icd_FSR::put_value(unsigned int new_value)
{
}

unsigned int icd_FSR::get(void)
{
    return get_value();
}
unsigned int icd_FSR::get_value(void)
{
    if(icd_fd<0) return 0;

    if(is_stale)
    {
	value = icd_cmd("$$7019\r")&0x00ff;
	is_stale=0;
	if(xref)
	    xref->update();
    }
    return(value);
}
icd_PCLATH::icd_PCLATH()
{
    replaced=0;
    value=0x42;
    is_stale=1;
};

void icd_PCLATH::put(unsigned int new_value)
{
}

void icd_PCLATH::put_value(unsigned int new_value)
{
}

unsigned int icd_PCLATH::get(void)
{
    return get_value();
}

unsigned int icd_PCLATH::get_value(void)
{
    if(icd_fd<0) return 0;

    if(is_stale)
    {
	value=(icd_cmd("$$701F\r")&0xff00)>>8;
	is_stale=0;
	if(xref)
	    xref->update();
    }
    return(value);
}
icd_PC::icd_PC()
{
    replaced=0;
    value=0x42;
    is_stale=1;
};

void icd_PC::put_value(unsigned int new_value)
{
}

unsigned int icd_PC::get_value(void)
{
    if(icd_fd<0) return 0;

    if(is_stale)
    {
	value = icd_cmd("$$701F\r");
	cpu->pcl->value = value & 0xff;
	cpu->pclath->value = value >> 8;

	is_stale=0;
    }
    return(value);
}

