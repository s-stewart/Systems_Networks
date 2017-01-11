#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "queue.h"
#include "network.h"
#include "rtp.h"

typedef struct _MESSAGE{
  char* buffer;
  int length;
} MESSAGE;

/* ================================================================ */
/*                  H E L P E R    F U N C T I O N S                */
/* ================================================================ */

/*
 *  Returns a number computed based on the data in the buffer.
 * 
 *
 */
static int checksum(char *buffer, int length){


  /*  ----  FIXME  ----
   *
   *  The goal is to return a number that is determined by the contents
   *  of the buffer passed in as a parameter.  There a multitude of ways
   *  to implement this function.  For simplicity, simply sum the ascii
   *  values of all the characters in the buffer, and return the total.
   */ 
   
   int checksum = 0;
   int buffer_index = 0;
   while (length != buffer_index){
		checksum += *buffer;
		buffer++;
		buffer_index++;
   }

  return checksum;

}

/*
 *  Converts the given buffer into an array of PACKETs and returns
 *  the array.  The value of (*count) should be updated so that it 
 *  contains the length of the array created.
 */
static PACKET* packetize(char *buffer, int length, int *count){

  /*  ----  FIXME  ----
   *
   *  The goal is to turn the buffer into an array of packets.
   *  You should allocate the space for an array of packets and
   *  return a pointer to the first element in that array.  The
   *  integer pointed to by 'count' should be updated to indicate
   *  the number of packets in the array.
   */ 
   
   int packet = 0;
   int payload_index = 0;
   int init = 1;
   int buffer_index = 0;
   PACKET *packets;
   
  while(buffer_index != length){												/*while there are still items in the buffer*/
  
	if (init == 1){															/*if i havent made any packets, i must make one to start*/
		packets = malloc(sizeof(PACKET));									/*allocate space for one packet*/
		*count = 1;														
	}

		
	
		if(payload_index < (MAX_PAYLOAD_LENGTH - 1)){							/*if there is still room in a packet, add it to that packet*/
			packets[packet].type = DATA;										/*set its type*/
			packets[packet].payload_length++;
			packets[packet].checksum = checksum(packets[packet].payload, packets[packet].payload_length);
			
			packets[packet].payload[payload_index] = buffer[buffer_index];		/*fill that payload index with the buffer*/
			payload_index++;
			buffer_index++;
		}
		
		else{																	/*if there is another item in the buffer that cant fit in this packet*/
			payload_index == 0;
			*count++;
			packets = realloc(packets,sizeof(PACKET)*(packet+1));						/*reallocate the space for another packet*/
			
			packets[packet].type = DATA;										/*start filling that packet*/
			packets[packet].payload_length++;
			packets[packet].checksum = checksum(packets[packet].payload, packets[packet].payload_length);
			packets[packet].payload[payload_index] = buffer[buffer_index];	
			payload_index++;
			buffer_index++;
		}
	
	
	/*change the last pack to LAST_DATA*/
	packets[packet].type = LAST_DATA;
	
	
    return packets;
  }
}

/* ================================================================ */
/*                      R T P       T H R E A D S                   */
/* ================================================================ */

static void *rtp_recv_thread(void *void_ptr){

    RTP_CONNECTION *connection = (RTP_CONNECTION*)void_ptr;

    do
    {
        MESSAGE *message = NULL;
        int buffer_length = 0;
		int i;
        char *buffer = NULL;
        PACKET packet;
		PACKET ACK;
		PACKET NACK;
		
		ACK.type = ACK;
		ACK.payload = 0;
		ACK.payload_length = 1;
		ACK.checksum = checksum(ACK.payload, ACK.payload_length);
		
		NACK.type = NACK;
		NACK.payload = 0;
		NACK.payload_length = 1;
		NACK.checksum = checksum(NACK.payload, NACK.payload_length);

        /* 
        * put messages in buffer until the last packet is received 
        */  
        do
        {
            char *temp = NULL;
            if (net_recv_packet(connection->net_connection_handle, &packet) != 1 || packet.type == TERM)
            {
	            /* remote side has disconnected */
	            connection->alive = 0;
	            pthread_cond_signal(&(connection->recv_cond));
	            pthread_cond_signal(&(connection->send_cond));
	            break;
            }

	            /*  ----  FIXME  ----
	            *
	            * 1. check to make sure payload of packet is correct 
	            * 2. send an ACK or a NACK, whichever is appropriate
	            * 3. if this is the last packet in a sequence of packets
	            *    and the payload was corrupted, make sure the loop
	            *    does not terminate
	            * 4. if the payload matches, add the payload to the buffer
	            *    as done below
	            */
				
			if ( checksum(&packet, packet.payload_length) == packet.checksum ){					/*if the checksum is correct, send ACK*/
				net_send_packet(connection->net_connection_handle, &ACK);
			}
			else{
				net_send_packet(connection->net_connection_handle, &NACK);
				if( packet.type == LAST_DATA ){
					packet.type = DATA;
				}
			}
			

            /*  ----  FIXME  ----
            * 
            *  What if the packet received is not a data packet?
            *  If it is a NACK or an ACK, the sending thread should
            *  be notified so that it can finish sending the message.
            *   
            *  1. add the necessary fields to the CONNECTION data structure
            *     in rtp.h so that the sending thread has a way to determine
            *     whether a NACK or an ACK was received
            *  2. signal the sending thread that an ACK or a NACK has been
            *     received.
            */
			if ( packet.type == ACK ){
				pthread_mutex_lock(&connection->ack_mutex);
				connection->ACK_received = 1;
				pthread_condition_broadcast(&connection->ack_cond);
				pthread_mutex_unlock(&connection->ack_mutex);
			}
			
			if ( packet.type == NACK ){
				pthread_mutex_lock(&connection->ack_mutex);
				connection->NACK_received = 1;
				pthead_condition_broadcast(&connection->ack_cond)
				pthread_mutex_unlock(&connection->ack_mutex);
			}
			
			
        } while (packet.type != LAST_DATA);

        if (connection->alive == 1)
        {
			/* --- FIX ME -----
             *
             * Add message to the received queue here.
             */
			
			if (buffer == NULL){
				buffer = malloc(packet.payload_length * sizeof(packet.payload));
				for (i = 0; i < (MAX_PAYLOAD_LENGTH - 1); i++){
					buffer[buffer_length] = packet[i].payload;
					buffer_length++;
				}
			}
			else{
				buffer = realloc(buffer, sizeof(buffer)+packet.payload_length);
				for (i = 0; i < (MAX_PAYLOAD_LENGTH - 1); i++){
					buffer[buffer_length] = packet[i].payload;
					buffer_length++;
				}
			}
        } else free(buffer);
    
    } while (connection->alive == 1);

    return NULL;

}

static void *rtp_send_thread(void *void_ptr){

    RTP_CONNECTION *connection = (RTP_CONNECTION*)void_ptr;
    MESSAGE *message = NULL;
    int array_length = 0, i = 0;
    PACKET *packet_array = NULL;
    
    do {
        /* extract the next message from the send queue */
        /* -------------------------------------------- */
        pthread_mutex_lock(&(connection->send_mutex));
        while (queue_size(&(connection->send_queue)) == 0 &&
               connection->alive == 1)
            pthread_cond_wait(&(connection->send_cond), &(connection->send_mutex));
        
        if (connection->alive == 0)
            break;
        
        message = queue_extract(&(connection->send_queue));
        
        pthread_mutex_unlock(&(connection->send_mutex));
        
        /* packetize the message and send it */
        /* --------------------------------- */
        packet_array = packetize(message->buffer, message->length, &array_length);
        
        for (i=0; i<array_length; i++) {
            
            /* Start sending the packetized messages */
            /* ------------------------------------- */
            if (net_send_packet(connection->net_connection_handle, &(packet_array[i])) < 0){
                /* remote side has disconnected */
                connection->alive = 0;
                break;
            }
            
            /*  ----FIX ME ---- 
             * 
             *  1. wait for the recv thread to notify you of when a NACK or
             *     an ACK has been received
             *  2. check the data structure for this connection to determine
             *     if an ACK or NACK was received.  (You'll have to add the
             *     necessary fields yourself)
             *  3. If it was an ACK, continue sending the packets.
             *  4. If it was a NACK, resend the last packet
             */

            pthread_mutex_lock(&connection->ack_mutex);
			while( connection->ACK_received == 0 && connection->NACK_received == 0){
				pthread_cond_wait(&connection->ack_cond, &connection->ack_mutex);
			}
			if ( connection->ACK_received == 1 ){
				connection->ACK_received = 0;
			}
			
			if ( connection->NACK_reveived == 1){
				connection->NACK_received = 0;
				i--;
			}
			
			pthread_mutex_unlock(&connection->ack_mutex);
        }
        
        free(packet_array);
        free(message->buffer);
        free(message);
    } while(connection->alive == 1);
    return NULL;
      

}

RTP_CONNECTION *rtp_init_connection(int net_connection_handle){
  RTP_CONNECTION *rtp_connection = malloc(sizeof(RTP_CONNECTION));
 
  if (rtp_connection == NULL){
    fprintf(stderr,"Out of memory!\n");
    exit (EXIT_FAILURE);
  }

  rtp_connection->net_connection_handle = net_connection_handle;

  queue_init(&(rtp_connection->recv_queue));
  queue_init(&(rtp_connection->send_queue));

  pthread_mutex_init(&(rtp_connection->ack_mutex), NULL);
  pthread_mutex_init(&(rtp_connection->recv_mutex),NULL);
  pthread_mutex_init(&(rtp_connection->send_mutex),NULL);
  pthread_cond_init(&(rtp_connection->ack_cond), NULL);
  pthread_cond_init(&(rtp_connection->recv_cond),NULL);
  pthread_cond_init(&(rtp_connection->send_cond),NULL);

  rtp_connection->alive = 1;

  pthread_create(&(rtp_connection->recv_thread), NULL, rtp_recv_thread,
		 (void*)rtp_connection);
  pthread_create(&(rtp_connection->send_thread), NULL, rtp_send_thread,
		 (void*)rtp_connection);

  return rtp_connection;
}

/* ================================================================ */
/*                           R T P    A P I                         */
/* ================================================================ */

RTP_CONNECTION *rtp_connect(char *host, int port){
  
  int net_connection_handle;

  if ((net_connection_handle = net_connect(host,port)) < 1)
    return NULL;

  return (rtp_init_connection(net_connection_handle));
}

int rtp_disconnect(RTP_CONNECTION *connection){

  MESSAGE *message;
  PACKET term;

  term.type = TERM;
  term.payload_length = term.checksum = 0;
  net_send_packet(connection->net_connection_handle, &term);
  connection->alive = 0;
  
  net_disconnect(connection->net_connection_handle);
  pthread_cond_signal(&(connection->send_cond));
  pthread_cond_signal(&(connection->recv_cond));
  pthread_join(connection->send_thread,NULL);
  pthread_join(connection->recv_thread,NULL);
  /* emtpy recv queue and free allocated memory */
  while ((message = queue_extract(&(connection->recv_queue))) != NULL){
    free(message->buffer);
    free(message);
  }

  /* emtpy send queue and free allocated memory */
  while ((message = queue_extract(&(connection->send_queue))) != NULL){
    free(message);
  }

  free(connection);

  return 1;

}

int rtp_recv_message(RTP_CONNECTION *connection, char **buffer, int *length){

  MESSAGE *message;

  if (connection->alive == 0) 
    return -1;
  /* lock */
  printf("Locking recv_mutex1\n");
  pthread_mutex_lock(&(connection->recv_mutex));
  printf("Locked recv_mutex1\n");
  while (queue_size(&(connection->recv_queue)) == 0 &&
	 connection->alive == 1)
    pthread_cond_wait(&(connection->recv_cond), &(connection->recv_mutex));
  
  if (connection->alive == 0) {
    printf("Unlocking recv_mutex2\n");
    pthread_mutex_unlock(&(connection->recv_mutex));
    printf("Unlocked recv_mutex2\n");
    return -1;
  }

  /* extract */
  message = queue_extract(&(connection->recv_queue));
  *buffer = message->buffer;
  *length = message->length;
  free(message);

  /* unlock */
  printf("Unlocking recv_mutex1\n");
  pthread_mutex_unlock(&(connection->recv_mutex));
  printf("Unlocked recv_mutex1\n");

  return *length;
}

int rtp_send_message(RTP_CONNECTION *connection, char *buffer, int length){

  MESSAGE *message;

  if (connection->alive == 0) 
    return -1;

  message = malloc(sizeof(MESSAGE));
  if (message == NULL)
    return -1;
  message->buffer = malloc(length);
  message->length = length;

  if (message->buffer == NULL){
    free(message);
    return -1;
  }

  memcpy(message->buffer,buffer,length);

  /* lock */
  printf("Locking send_mutex2\n");
  pthread_mutex_lock(&(connection->send_mutex));
  printf("Locked send_mutex2\n");
  
  /* add */
  queue_add(&(connection->send_queue),message);

  /* unlock */
  printf("Unlocking send_mutex2\n");
  pthread_mutex_unlock(&(connection->send_mutex));
  printf("Unlocked send_mutex2\n");
  pthread_cond_signal(&(connection->send_cond));
  return 1;

}
