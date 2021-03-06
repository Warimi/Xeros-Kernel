/* kbc.c : Keyboard
 */

#include <xeroskernel.h>
#include <stdarg.h>
#include <kbd.h>


void addCharToBuffer(char character);
void echoON( void );
void echoOFF( void );

short isKeyboardDataReady( void );
int 			ENTERKEY = 0xa;
int 			CTL_D = 0x4;
dvfunc   		deviceTable[2];
unsigned char 	pressed;
unsigned char 	keyboardBuffer[4];
unsigned char 	eofChar;
Bool 			isEchoing;





extern int kbdopen(pcb* process, dvfunc* device, int dvnum) {
    kprintf("kbdopen()\n");

	int fdnum = dvnum;
    
    fdt* echoKbd = &process->fileDescriptorTable[KBD_ECHO];
    fdt* nonEchoKbd = &process->fileDescriptorTable[KBD_NON_ECHO];
    
    if (echoKbd->status || nonEchoKbd->status) {
        kprintf("A device is already open. Returning error\n");
        return -1;
    } else {
        kprintf("Opening keyboard %d (0 = nonecho, 1 = echo)\n", dvnum);

		// although we know we only have 2 devices to deal with, which is echo and non-echo keyboard
		// in reality, we don't know how many devices we would be dealing with
		// there for instead of just setting up if( dvnum ), we make it general by case checking        
		if( dvnum == 0 ) {
			echoOFF();
		}

		if( dvnum == 1 ) {
			echoON();
		} 

        enable_irq(1,0);
		process->fileDescriptorTable[fdnum].status = DEVICE_OPENED;
    }
    
    return fdnum;
}

extern int kbdclose(pcb* process, dvfunc* device, int dvnum) {
    kprintf("kbdclose()\n");
    kprintf("Closing keyboard %d (0 = nonecho, 1 = echo)\n", dvnum);
    enable_irq(1,1);
	process->fileDescriptorTable[dvnum].status = DEVICE_CLOSED;
    return 0;
}

// writes are not supported by keyboards, return -1 automatically
extern int kbdwrite( void *buff, int bufflen ) {
    return -1;
}

extern int kbd_handler( void ) {
    if(isKeyboardDataReady()) {
        unsigned char fromPort = inb(KBDPORT1);
        unsigned char character = kbtoa(fromPort);
        
        if ((int) character == eofChar) {
            addCharToBuffer(pressed);
            kprintf("%c", pressed);
			return 0;
		} else if((int)character == CTL_D) {
			
        } else if(character == ENTERKEY) {
            pressed = '\n';
            addCharToBuffer(pressed);
            kprintf("%c", pressed);
			return 0;
        } else if(character) {
            kprintf("%c", character);
            pressed = character;
            addCharToBuffer(character);
			return 0;
    	}
    
    	return 0;
	}

	return 0;

}

extern int kbdRead(void* buff, unsigned int bufflen) {

	if(isKeyboardDataReady()) {
        unsigned char fromPort = inb(KBDPORT1);
        unsigned char character = kbtoa(fromPort);
        
		if (character == eofChar) {
    		return 0;
    	} else if(character == CTL_D) {
    		return 0;
    	} else if(character == ENTERKEY) {
       		pressed = '\n';
       		addCharToBuffer(pressed);
       		memset(buff, pressed, bufflen);
       		if (isEchoing){
       			kprintf("%c", pressed);
       			}
			return 0;
    	} else {
    		if(isEchoing){
       			kprintf("%c", character);
       			}
       		pressed = character;
       		memset(buff, pressed, bufflen);
       		addCharToBuffer(character);
    			}
		}
	return 0;
}


extern int kbdioctl(unsigned long command, char newEofChar) {
	if(command == 53){
	eofChar = newEofChar;
	return 0;
	}
	
	if(command == 55){
	echoOFF();
	kprintf( "echo mode is now OFF\n");
	return 0;
	}
	
	if (command == 56){
	echoON();
	kprintf( "echo mode is now ON\n" );
	return 0;
	}

	// if we get to this point, we have an invalid command
	kprintf( "invalid command %d\n", command );	
	return -1;
}



void echoON( void ) {

	isEchoing = TRUE;

}

void echoOFF( void ) {

	isEchoing = FALSE;

}

// insert character into keyboardBuffer[]
void addCharToBuffer(char character) {
    int i;
    for (i = 0; i < 4; i++) {
        if (! keyboardBuffer[i]) {
            keyboardBuffer[i] = character;
        }
    }
}


short isKeyboardDataReady( void ) {
    return (inb(KBDPORT2) & 0x01);
}


// source code provided by instructor in scancodeToAscii.txt
static int extchar( unsigned char code ){
	return state &= ~EXTENDED;
}

unsigned int kbtoa( unsigned char code ){
  unsigned int    ch;
  
  if (state & EXTENDED)
    return extchar(code);
  if (code & KEY_UP) {
    switch (code & 0x7f) {
    case LSHIFT:
    case RSHIFT:
      state &= ~INSHIFT;
      break;
    case CAPSL:
      kprintf("Capslock off detected\n");
      state &= ~CAPSLOCK;
      break;
    case LCTL:
      state &= ~INCTL;
      break;
    case LMETA:
      state &= ~INMETA;
      break;
    }
    
    return NOCHAR;
  }
  
  
  /* check for special keys */
  switch (code) {
  case LSHIFT:
  case RSHIFT:
    state |= INSHIFT;
    kprintf("shift detected!\n");
    return NOCHAR;
  case CAPSL:
    state |= CAPSLOCK;
    kprintf("Capslock ON detected!\n");
    return NOCHAR;
  case LCTL:
    state |= INCTL;
    return NOCHAR;
  case LMETA:
    state |= INMETA;
    return NOCHAR;
  case EXTESC:
    state |= EXTENDED;
    return NOCHAR;
  }
  
  ch = NOCHAR;
  
  if (code < sizeof(kbcode)){
    if ( state & CAPSLOCK )
      ch = kbshift[code];
	  else
	    ch = kbcode[code];
  }
  if (state & INSHIFT) {
    if (code >= sizeof(kbshift))
      return NOCHAR;
    if ( state & CAPSLOCK )
      ch = kbcode[code];
    else
      ch = kbshift[code];
  }
  if (state & INCTL) {
    if (code >= sizeof(kbctl))
      return NOCHAR;
    ch = kbctl[code];
  }
  if (state & INMETA)
    ch += 0x80;
  return ch;
}
