PROJECT_PATH = ~/Dev/Pebble/TextWatch

c:
	./waf configure && ./waf build
l: c
	deploypebble.sh load $(PROJECT_PATH)/build/TextWatch.pbw
d: c
	deploypebble.sh reinstall  $(PROJECT_PATH)/build/TextWatch.pbw 
