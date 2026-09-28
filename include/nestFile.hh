#ifndef NESTPART_HH
#define NESTPART_HH

#include "VDetector.hh"  // NEST abstract interface

extern double voltage;
extern double b;
extern double a;

class nestPart : public VDetector {
public:
    nestPart();
    ~nestPart() override = default;

    double get_ElectricField(double x = 0., double y = 0., double z = 0.) const;
    double get_DetectorDensity() const;
    double get_WLspec() const;
    double get_dopedLXeConcentration() const;
    bool get_inGas() const;
    bool get_doFieldDependentYield() const;
};

#endif
