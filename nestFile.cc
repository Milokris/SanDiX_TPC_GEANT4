#include "nestFile.hh"
#include <cmath>

nestPart::nestPart() : VDetector() 
{}

double voltage = 4750.0;         // volts
double a = 12.5 * pow(10, -4);   // cm, inner electrode radius 12.5*um
double b = 2.5;            // cm, outer electrode radius 25*mm

double nestPart::get_ElectricField(double x, double y, double z) const 
{
    double r = std::sqrt(x*x + y*y);

    if (r < a || r > b) return 0.0;   // outside active volume
    return voltage / (r * std::log(b / a)); // in V/cm
}
