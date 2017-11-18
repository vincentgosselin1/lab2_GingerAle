#buffer.o nom du .c 
obj-m        := char_user.o

all: ele784_driver

# We build our module in this section
ele784_driver:
	@echo "Building cam_user"
	gcc cam_user.c -o cam_user

#clean:
#    -rm -f *.tmp_versions
