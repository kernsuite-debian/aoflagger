#ifndef TICKSET_H
#define TICKSET_H

#include <string>

#ifndef HAVE_EXP10
#define exp10(x) exp((2.3025850929940456840179914546844) * (x))
#endif

typedef std::pair<double, std::string> Tick;

class TickSet {
 public:
  TickSet() {}
  virtual ~TickSet() {}

  virtual size_t Size() const = 0;
  virtual Tick GetTick(size_t i) const = 0;

  virtual std::unique_ptr<TickSet> Clone() const = 0;

  virtual void DecreaseTicks() {
    if (Size() > 1) {
      Set(Size() - 1);
    }
  }
  virtual void Set(size_t maxSize) = 0;
  virtual void Reset() = 0;
  /**
   * Returns a value scaled according to the axis.
   * Values that are within the min and max will have an axis value of
   * 0 to 1.
   */
  virtual double UnitToAxis(double unitValue) const = 0;
  virtual double AxisToUnit(double axisValue) const = 0;

 protected:
 private:
};

#endif
