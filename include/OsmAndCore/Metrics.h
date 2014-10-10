#ifndef _OSMAND_CORE_METRICS_H_
#define _OSMAND_CORE_METRICS_H_

#define EMIT_METRIC_FIELD(type, name, measurement) \
    type name
#define RESET_METRIC_FIELD(type, name, measurement) \
    name = 0
#define PRINT_METRIC_FIELD(type, name, measurement) \
    output += (output.isEmpty() ? QString() : QString(QLatin1String("\n"))) + prefix + QString(QLatin1String(#name " = %1" measurement)).arg(name)

#endif // !defined(_OSMAND_CORE_METRICS_H_)
