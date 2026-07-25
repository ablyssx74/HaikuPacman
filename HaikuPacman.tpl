name			$(TARGET)
version			$(VERSION)-1
architecture	$(ARCH)
summary 		"HaikuPacman"
description 	"HaikuPacman - Pacman game"
packager		"ablyss <HaikuPacman@epluribusunix.net>"
vendor			"epluribusunix.net Project"
licenses {
	"MIT"
}
copyrights {
	"$(YEAR) ablyss"
}
provides {
	$(TARGET) = $(VERSION)-1
	libhdhomerun
}
requires {
	haiku
	libsdl2
	curl
}	
urls {
	"https://github.com/ablyssx74/HaikuPacman"
}
