Oct 27, 2000 - Updated Dec 23, 2001
Copyright, JMH Associates, Inc., 2000, 2001.
Johnson M. Hart, jmhart@world.std.com
http://world.std.com/~jmhart

Message Queue Emulation Architecture and Implementation

MQ OVERVIEW

The intent of this system is to emulate the four UNIX message 
queue (MQ) functions: 
	1) msgsnd
	2) msgrcv
	3) msgget
	4) msgctl
on any platform that supports Pthreads as well as Windows. The system
has been tested on Windows (9x, Me, NT, 2000, but XP even though there
is no reason why it shouldn't work on XP) on Compaq OpenVMS.

The MQ functions allow for message types (hence priorities) within a 
FIFO discipline, and each individual MQ is identified by a user-defined 
"key" which is similar to a file name. Then, msgget() converts the key 
to a MQ ID, which is somewhat similar to a file descriptor. msgsnd() 
and msgrcv() are for sending and receiving messages, which are typed 
with what amounts to a priority, so that msgrcv() can extract a message 
of a specified type or of type below a threshold. msgctl() is used to 
set attributes of a specified MQ.

Typed message applications include:

  1) A specification of the intended receiver(s). That is, receiver
     threads can wait for a message with a specific type value.
     This provides a convenient demultiplexing method.
  2) The type can represent a priority, and a receiving thread can
     access any message of sufficiently high priority (low type number
     means high priority).

The man pages on any UNIX system provide abundant additional information.

EMULATION OVERVIEW

The emulation is written so that it will run on Windows as well as
on Pthreads platforms. This is achieved by using macros (see the file
thread_emulation.h) that emulate the required Pthread functionality
under Win32. You can also use (with a few source code changes so as
to #include "pthread.h") the Open Source pthread emulation library,
available from (among other places):

http://sources.redhat.com/pthreads-win32/

Also, see my comments on the Open Source pthreads library at:
http://world.std.com/~jmhart/opensource.htm

NOTE: Windows programmers who would prefer not to have the unfamiliar
Pthread functions in the code are welcome to replace the functions with
their macro equivalents, as defined in thread_emulation.h


The component pieces of the emulation, WHICH CORRESPOND EXACTLY TO WINDOWS
PROJECTS AND TO THE EXCECUTABLES OR LIBARIES, are:

1)	msg_emulation - library
This is the collection of thread-safe MQ emulation functions.
The four public entry points are msgsnd, msgrcv, msgget, msgctl.

2) msg_emulation_test: This is test MQ client.


---------------------

LIMITS: 

There are several built-in limits, all controlled by macros.

Maximum number of message queues: 256
  Macro: MSG_MAX_NUMBER_QUEUES in msq_emulation_internal.h
  Note: MQs can be destroyed with a msgctl call, so the limit only 
applies to the number of active queue keys at any one time

Maximum number of bytes in a single queue message: 65K
  Macro: MQ_MAX_MESSAGE_SIZE	in msq_emulation_internal.h	

Maximum total number of bytes in a single queue: 
		64 * MQ_MAX_MESSAGE_SIZE = 2**22 = 4 MB
  Macro: MQ_MAX_BYTES_IN_QUEUE		

Maximum total number of bytes in all queues:
	MQ_MAX_BYTES_IN_QUEUE*MSG_MAX_NUMBER_QUEUES/2 = 2**29 = 512 MB
		This looks too large! We may need to decrease it

The security features are not properly emulated.

The message queues are only shared by the threads of a process, rather
than being accessible to threads in multiple processes. A correct emulation
would required shared memory; for an example of process shared objects
in a Windows environment, see: http://world.std.com/~jmhart/mltwtsm.htm
* * * *
ADDITIONAL ENHANCEMENT POSSIBILITIES

1. The implementation is not efficient in the sense that the message
queues are maintained as simple sequential structures that are searched
sequentially. This is not bad in most cases, but, with lots of messages
sent by lots of threads, performance could degrade. An interesting
enhancement would use a C++ Standard Template Library (STL) container.

2. Implement process sharing (see the note at the end of the preceding
paragraph).

3. Create an open source version of this library, primarily for Windows 
users. I don't have the time, but would be happy to support any such effort
in any way that I can. Such a library would be very useful when porting
UNIX programs to Windows, and, in addition, the functionality could be
useful when writing new applications.

