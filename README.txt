So this is how I see this example working:

Conceptual idea:
3 threads: main (constructs the other 2), producer, consumer
Main makes all the stuff then goes to sleep until the other 2 terminate
Producer produces some data, which is sent to the consumer through a buffer
Consumer consumes and validates the data, and then sends an acknowledgement indicating receipt of the data
Producer then makes a new piece of data, and the cycle continues

Process:
Producer makes a piece of data according to a predictable algorithm (e.g. Nt+1 = Nt++)
Producer gets the P->C buffer mutex lock
Producer puts the data in the buffer
Producer releases the P->C buffer mutex lock
Then Producer waits until it gets an ACK from the Consusmer (by constantly getting the C->P buffer mutex lock and checking for an update, then releasing the lock, over and over again)
Meanwhile, Consumer is constantly getting the P->C buffer mutex lock and checking for an update, then releasing the lock, over and over again
When it detects updated data, it copies the data locally and clears the buffer (so it doesn't think there's new data on subsequent cycles), then releases the lock
The Consumer compares the sent data to what it was expecting to receive (according to the algorithm)
Either way, the Consumer gets the C->P buffer mutex lock, puts an ACK in the buffer (if the data was what it was expecting), or a NAK (if the data was not what it was expecting), and then releases the C->P buffer mutex lock
Then the Consumer goes back to constantly checking the P->C buffer
The Producer should receive the ACK/NAK from the Consumer via the C->P buffer (because it was constantly checking), and will then generate the next piece of data
