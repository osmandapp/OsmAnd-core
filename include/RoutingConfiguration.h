/*
 * RoutingConfiguration.h
 *
 *  Created on: Mar 4, 2013
 *      Author: victor
 */

#ifndef ROUTINGCONFIGURATION_H_
#define ROUTINGCONFIGURATION_H_
#include <qiodevice.h>
#include <qstring.h>
#include <qmap.h>

#include <OsmAndCore.h>
#include <RoutingVehicleConfig.h>

namespace OsmAnd {

class OSMAND_CORE_API RoutingConfiguration {
public:
	struct RoutingConfigurationFile {
		QMap<QString, std::shared_ptr<RoutingVehicleConfig> > routers;
		QMap<QString, QString> attributes;
		QString defaultRouterName;
	};

	QMap<QString, QString> attributes;

	// 1. parameters of routing and different tweaks
	// Influence on A* : f(x) + heuristicCoefficient*g(X)
	const float heuristicCoefficient;

	// 1.1 tile load parameters (should not affect routing)
	const int zoomToLoadTiles;
	const int memoryLimitation;

	// 1.2 Build A* graph in backward/forward direction (should not affect results)
	// 0 - 2 ways, 1 - direct way, -1 - reverse way
	const int planRoadDirection;

	// 1.3 Router specific attributes, coefficients and restrictions
	const RoutingVehicleConfig router;
	const QString routerName;

	// 1.4 Used to calculate route in movement (-720 when not set)
	const double initialDirection;

	// 1.5 Recalculate distance help
	const double recalculateDistance;

	static RoutingConfigurationFile parseFromInputStream(QIODevice* is);

private:
	QString getAttribute(RoutingVehicleConfig& router, QString& propertyName) ;

	int parseSilentInt(QString t, int v) ;

	float parseSilentFloat(QString t, float v) ;
public :
	RoutingConfiguration(std::shared_ptr<RoutingConfigurationFile> file, QString router, double direction = -720,
			int memoryLimitMB = 30, QMap<QString, QString> params) ;

	virtual ~RoutingConfiguration();

};

} /* namespace OsmAnd */
#endif /* ROUTINGCONFIGURATION_H_ */
