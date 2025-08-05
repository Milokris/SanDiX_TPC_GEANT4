#include "nestFile.hh"
#include <cmath>

nestPart::nestPart() : VDetector() 
{}

double nestPart::get_ElectricField(double x, double y, double z) const 
{
    double r = std::sqrt(x*x + y*y);
    const double V = 10000.0;         // volts
    const double a = 12.5 * pow(10, -4);   // cm, inner electrode radius 12.5*um
    const double b = 2.5;            // cm, outer electrode radius 25*mm

    if (r < a || r > b) return 0.0;   // outside active volume
    return V / (r * std::log(b / a)); // in V/cm
}
