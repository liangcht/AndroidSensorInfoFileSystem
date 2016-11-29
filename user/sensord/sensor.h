#ifndef _SENSOR_H
#define _SENSOR_H

/* Please use the same syscall number as indicated in the homework 6 instruction */

#define __NR_set_sensor_information 244
#define GPS_LOCATION_FILE "/data/media/0/gps_location.txt"

struct sensor_information {
    int microlatitude; /* GPS Latitude in millionths */
    int microlongitude; /* GPS Longitude in millionths */
    int centilux; /* Luminosity in hundredths */
    int centiproximity; /* Proximity in hundredths */
    int centilinearaccelx;
    /* Linear acceleration in the x dimension in hundredths */
    int centilinearaccely;
    /* Linear acceleration in the y dimension in hundredths */
    int centilinearaccelz;
    /* Linear acceleration in the z dimension in hundredths */
};

#endif
