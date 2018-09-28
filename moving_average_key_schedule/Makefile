build: acm.p4 plugin.c
	/opt/netronome/p4/bin/nfp4build -p out/ -o acm.nffw -r -e -l starfighter1 -4 acm.p4 -c plugin.c

clean:
	rm -rf *.list *.nffw ./out *.yml *~ Makefile-nfp4build
