
This program is made to reduce the amount of brute force attacks on ssh
servers. The standard ways to reduce this types of activity include running the
server on non-standard port, using only public-key authentication, and adding
longer timeout after a set amount of failed login attempts.

I have chosen a different approach. This program will run start ssh server
after the client has "knocked" on three ports in correct order. The ports are
do not respond anything. After a set timeout the ssh server will stop.

