# Main settings includes
include	C:\Espressif\examples\ESP8266/settings.mk

# Individual project settings (Optional)
#BOOT		= new
#APP		= 1
#SPI_SPEED	= 40
#SPI_MODE	= QIO
SPI_SIZE_MAP	= 4
ESPPORT		= COM6
ESPBAUD		= 115200

# Basic project settings
MODULES	= driver user
LIBS	= c gcc hal phy pp net80211 lwip wpa crypto main upgrade json pwm ssl

# Root includes
include	C:\Espressif\examples\ESP8266/common_nonos.mk
