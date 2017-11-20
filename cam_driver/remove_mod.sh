#script to remove default drivers for the camera
sudo modprobe -r uvcvideo
sudo modprobe -r v4l2_common
sudo modprobe -r videodev
sudo modprobe -r videobuf2_vmalloc
sudo modprobe -r videobuf2_memops
sudo modprobe -r videobuf2_core
sudo modprobe -r snd_usb_audio #is used?
sudo modprobe -r snd_usbmidi_lib #is used?
