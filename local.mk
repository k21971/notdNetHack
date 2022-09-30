GAMEDIR = /opt/nethack/chroot/notdnethack-2022.09.18
CFLAGS = -g3 -O0 -fcommon -Wno-format-overflow
CPPFLAGS = -DWIZARD=\"build\" -DCOMPRESS=\"/bin/gzip\" -DCOMPRESS_EXTENSION=\".gz\"
CPPFLAGS += -DHACKDIR=\"/notdnethack-2022.09.18\" -DDUMPMSGS=100
CPPFLAGS += -DDUMP_FN=\"/dgldir/userdata/%N/%n/notdnethack/dumplog/%t.ndnh.txt\"
CPPFLAGS += -DHUPLIST_FN=\"/dgldir/userdata/%N/%n/notdnethack/hanguplist.txt\"
CPPFLAGS += -DEXTRAINFO_FN=\"/dgldir/extrainfo-ndnh/%n.extrainfo\"