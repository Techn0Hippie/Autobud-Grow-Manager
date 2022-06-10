# Autobud-Grow-Manager
Interface Functions 
Auto Water
Auto Water: Frequency that the AGM initiates a water action (in hours)
Water Amount: Amount if water to deliver (in oz)
Water Calibration: The amount of time (in seconds) that the pump operates to deliver 1 oz of water. This will need to be determined by the user based on the setup.
	To Calibrate:
Using a measuring device, use a stop watch to time how long it takes to to deliver 16 oz of water (in seconds). Divide that number by 16 to get the water calibration number. 
Water Tank Volume: The capacity of the water reserve tank (in oz)
Dry Soil Threshold: The target soil moisture content where the device will initiate a water event. (Requires external sensor). 

Light Recording
The amount of time (in hours) the internal lighting is on (will be added in next release to control separate relay)

Auto Override
Water: Initiate an on demand watering. Will water based on Amount & Calibration 
Refill Water Tank: Sets the tank level to its capacity
Grow Hour Shift: Overrides the AGMs internal hour clock. Useful when changing lighting times (for example, if you want the lights to come on at 8AM and its 10AM right now, you want to set the current hour to 2. Lights will always start at hour 1).
Grow Day Shift: Shifts the Grow Day. Useful when moving plants between grow areas

Connections & Configuration 
WiFi SSID: Name of your SSID for WiFi
WiFi Password: Password for WiFi
(note, you must reboot the controller for this to take effect. Wifi operations happen at bottup)
Below features will be explained when Datacenter is released 
API Endpoint: Endpoint where controller posts data
Alert Text/Email: Address where notifications are sent
Device ID: ID of device for recording processes 
Pump GPIO: GPIO of connected pump to microcontroller (will depend on device)

Grow Status:
DeviceD: ID of device
Grow Day: How many days the plant has lived (from seed) 
Total Days in Veg: Days spent where lighting interval has been greater than 12 hours per day
Total Days in Flower: Days spent where lighting interval is 12 hours or less
Hour: Grow hour
Watering in: Time in hours before next watering
Water tank Level: Remaining fluid in oz (only works where runoff is not re circulated into tank and device is properly calibrated) 

Sensor Data (requires external Sensor)
Shows data received by external sensor

