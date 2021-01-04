all: ssu_crontab ssu_crond ssu_rsync

ssu_crontab: ssu_crontab.o
	gcc ssu_crontab.o -o ssu_crontab
ssu_crontab.o: ssu_crontab.c
	gcc -c ssu_crontab.c -o ssu_crontab.o

ssu_crond: ssu_crond.o
	gcc ssu_crond.o -o ssu_crond
ssu_crond.o: ssu_crond.c
	gcc -c ssu_crond.c -o ssu_crond.o

ssu_rsync: ssu_rsync.o
	gcc ssu_rsync.o -o ssu_rsync
ssu_rsync.o: ssu_rsync.c
	gcc -c ssu_rsync.c -o ssu_rsync.o

clean:
	rm *.o core ssu_crontab ssu_crond ssu_rsync
