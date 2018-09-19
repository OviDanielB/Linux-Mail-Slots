# Linux MailSlots Module

This project relates to the implementation of a special device file accessible according to FIFO style semantic in which every segment are 
posted and delivered atomically and in data separation to the reading threads. 

The device file is multi-instance and the parameters(blocking and/or non blocking read and write operations, maximum message size and maximum 
mail storage) of every instance can be configured with **ioctl** sys call.