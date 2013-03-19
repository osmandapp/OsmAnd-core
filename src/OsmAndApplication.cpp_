/*
 * OsmAndApplication.cpp
 *
 *  Created on: Mar 15, 2013
 *      Author: victor
 */

#include "OsmAndApplication.h"

namespace OsmAnd {

OsmAndApplication::OsmAndApplication() {
}

OsmAndApplication::~OsmAndApplication() {
}

std::shared_ptr<OsmAndApplication> globalApplicationRef;
std::shared_ptr<OsmAndApplication> OsmAndApplication::getAndInitializeApplication() {
	if (globalApplicationRef.get() == nullptr) {
		globalApplicationRef = std::shared_ptr<OsmAndApplication>(new OsmAndApplication());
	}

	return globalApplicationRef;
}

} /* namespace OsmAnd */
