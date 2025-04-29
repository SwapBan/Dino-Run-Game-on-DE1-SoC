oot@de1-soc:/# dmesg | tail -30
[    1.525884] usb 1-1: new high-speed USB device number 2 using dwc2
[    1.776573] hub 1-1:1.0: USB hub found
[    1.780425] hub 1-1:1.0: 2 ports detected
[    2.163312] random: fast init done
[    4.938550] EXT4-fs (mmcblk0p2): recovery complete
[    5.854513] random: crng init done
[    5.924551] EXT4-fs (mmcblk0p2): mounted filesystem with ordered data mode. Opts: (null)
[    5.932653] VFS: Mounted root (ext4 filesystem) on device 179:2.
[    6.120429] devtmpfs: mounted
[    6.125743] Freeing unused kernel memory: 1024K
[    6.130449] Run /sbin/init as init process
[   11.893421] systemd[1]: System time before build time, advancing clock.
[   12.187393] systemd[1]: systemd 229 running in system mode. (+PAM +AUDIT +SELINUX +IMA +APPARMOR +SMACK +SYSVINIT +UTMP +LIBCRYPTSETUP +GCRYPT +GNUTLS +ACL +XZ -LZ4 +SECCOMP +BLKID +ELFUTILS +KMOD -IDN)
[   12.205712] systemd[1]: Detected architecture arm.
[   12.771161] systemd[1]: Set hostname to <de1-soc>.
[   16.995005] systemd[1]: Listening on /dev/initctl Compatibility Named Pipe.
[   17.026022] systemd[1]: Reached target Swap.
[   17.055997] systemd[1]: Reached target Remote File Systems (Pre).
[   17.085978] systemd[1]: Reached target Remote File Systems.
[   17.259407] systemd[1]: Started Forward Password Requests to Wall Directory Watch.
[   17.296208] systemd[1]: Listening on Journal Socket.
[   17.326214] systemd[1]: Created slice User and Session Slice.
[   17.356143] systemd[1]: Listening on udev Control Socket.
[   17.386128] systemd[1]: Created slice System Slice.
[   17.416201] systemd[1]: Reached target Slices.
[   17.446190] systemd[1]: Created slice system-getty.slice.
[   17.477486] systemd[1]: Starting Create Static Device Nodes in /dev...
[   17.655684] systemd[1]: Starting Load Kernel Modules...
[   17.686374] systemd[1]: Started Dispatch Password Requests to Console Directory Watch.
[   21.838012] systemd-journald[784]: Received request to flush runtime journal from PID 1
