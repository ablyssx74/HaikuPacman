name			$(TARGET)
version			$(VERSION)-$(REVISION)
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
	$(TARGET) = $(VERSION)-$(REVISION)
}
requires {
	haiku
	libsdl2$(is32bit)
	curl$(is32bit)
}	
urls {
	"https://github.com/ablyssx74/HaikuPacman"
}
