#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

struct sensor_information {
	int microlatitude;
	int microlongitude;
	int centilux;
	int centiproximity;
	int centilinearaccelx;
	int centilinearaccely;
	int centilinearaccelz;
};

#endif /* SENSOR_TYPES_H */
