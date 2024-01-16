# esp8266-aprs-WX
bmp280 &amp; aht20 APRS-Uploader

This code collects weather data from two sensors: the AHT20 and the BMP280. The AHT20 sensor measures temperature and humidity, while the BMP280 sensor measures temperature and pressure. Both temperature values from the two sensors are averaged to obtain a more accurate temperature reading. The code subtracts an offset value from the average temperature to calibrate it.

After obtaining the weather data values (average temperature, pressure, and humidity), the code establishes a connection to an APRS server using a TCP/IP connection. It then sends the weather data to the APRS server in a specific format. The temperature is converted to Fahrenheit, and the pressure and humidity values are adjusted by applying offset values. The code also includes some debugging information to verify the collected data. The APRS server receives the weather data and broadcasts it to other APRS users. Additionally, the code includes functionality for OTA firmware updates, enabling the device to be remotely updated, and hosts a web page for local monitoring of the weather data.
