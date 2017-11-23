#Sweet shortcut to get dmesg in a console
#watch -n 0,1 "dmesg | grep cam_driver | tail "
watch -n 0,1 "dmesg | grep cam_driver | tail -n $((LINES-40))"
#watch -n 0,1 "dmesg | grep cam_driver"
