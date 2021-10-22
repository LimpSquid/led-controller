#include <bus.h>

/*
 * @Todo: implement bus protocol
 * 
 * We need no complexity at all for our bus protocol. A message might look like as follows:
 *
 *  master: |address byte|command byte|data byte 1|data byte 2|data byte 3|data byte 4|crc byte 1|crc byte 2|
 *  slave:  |response status byte|data byte 1|data byte 2|data byte 3|data byte 4|crc byte 1|crc byte 2|
 * 
 * The protocol uses a fixed message width with 4 data bytes and two CRC bytes. If the relevant
 * command doesn't use any of the data bytes, zeroing these fields will suffice.
 * 
 * Because we use a fixed message width we don't need any message idle time to indicate a tranmission
 * is completed. We can simply listen to the incoming stream of bytes and always assume the format
 * as described above.
 * 
 * In case a CRC fails is detected on a slave node, the slave should backoff for a fixed to 
 * be determined time and clear its receive buffer afterwards. Once the backoff time has expired
 * and the receive buffer is cleared, the node should operate as normal again.
 * 
 * Only the master node should retry sending messages. It should wait for up to atleast the 2x backoff
 * period(s) until it may retry sending a message after this period has expired. This ensures that in case of a CRC
 * failure on the slave's end, the master won't send any data during the backoff period. In case the master
 * encounters a CRC error, it may retry sending the message immediatly.
 * 
 * Consider the following situation. Master sends command to slave, slave receives and executes command, 
 * slave sends back response, response gets lost.  The master will now retry sending the same command, 
 * by which it will likely be executed twice by the slave node. For now this behaviour is acceptable and
 * we will not design the protocol to be idempotent, until some new feature requires it to be. 
 *
 * Broadcasts should be supported by all slaves and can be distinguished by a reserved address (like 255). 
 * No slave should ever reply to a broadcast message.
 */