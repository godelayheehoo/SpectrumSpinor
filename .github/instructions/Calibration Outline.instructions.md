---
applyTo: '**'
---
We are going to need to work in a calibration routine for each of our four sensors A,B,C, and D.

We'll access this by having a "calibration" menu item that will open into a choice of "A","B", "C", or "D" sensor calibration.

These will then have the options for "Set Dark", "Set White", and "Set Color X" for each of the colors we want to calibrate.

We will be doing calibration according to the below procedure:

Short answer: white-balancing + normalizing gets you most of the way there, but for fine distinctions (brown/yellow/orange/pink/purple) you’ll want to convert to a hue/saturation/value style representation (or use a tiny classifier) and use both hue and saturation/value (brightness) thresholds. Brown is basically dark/orange (same hue as orange but low value), pink is desaturated/red with high value, purple is high-magenta hue, yellow sits near the green→red boundary with high value/saturation, etc. Note where it specifies 10 calibration samples though, let's do 20. 

-----

Below is a practical, robust recipe you can implement on the ESP32 + TCS34725 + TCA9548A setup, plus Arduino-style code you can drop into your sketch.

1) Calibration steps (one time per sensor)

Dark offset — cover sensor, read R,G,B,C and store those as R_dark, G_dark, B_dark, C_dark. Subtract these from all future raw readings.

White balance — put a neutral white under sensor, read Rw, Gw, Bw. Compute gains:

avg = (Rw + Gw + Bw) / 3.0
r_gain = avg / Rw
g_gain = avg / Gw
b_gain = avg / Bw


Save r_gain,g_gain,b_gain for that sensor.

(Optional) Capture a few labeled color samples for each color class if you plan to train a small classifier.

Store these values per sensor in EEPROM / little JSON on SPIFFS / array in flash.

2) Runtime processing for each reading

Read raw Rraw,Graw,Braw, Craw.

Subtract dark: R = max(0, Rraw - R_dark) etc.

Apply white gains: R *= r_gain etc.

Normalize for brightness: r = R / C; g = G / C; b = B / C (use C after dark subtraction; if C is tiny, skip / treat as very low light).

Alternatively normalize to sum: sum = r+g+b; r/=sum; g/=sum; b/=sum — both are fine; dividing by C keeps photometric behavior.

Convert normalized RGB to HSV (or compute hue directly). Use the standard RGB→HSV formulas (see code below).

Use hue (°) + saturation + value thresholds or a nearest-centroid classifier to assign color.

3) Suggested hue / sat / val ranges (starting points)

These are starting heuristics; you must tune per sensor:

Red: hue ∈ [345°, 360) ∪ [0°, 15°], sat > 0.35, val > 0.15

Orange: hue ∈ [15°, 45°], sat > 0.35, val > 0.15

Yellow: hue ∈ [45°, 75°], sat > 0.30, val > 0.25

Green: hue ∈ [75°, 165°], sat > 0.25, val > 0.12

Blue: hue ∈ [165°, 270°], sat > 0.25, val > 0.06

Purple: hue ∈ [270°, 315°], sat > 0.20, val > 0.06

Pink: hue ≈ red range but lower sat and higher val (sat 0.15–0.45, val > 0.5) — basically bright, desaturated red.

Brown: hue in orange range (~20°–40°) but low val (val < 0.25) and moderate sat. Brown = dark orange.

These overlap; use priority rules (e.g., check brown before orange, pink before red) or a classifier.

4) Simple nearest-centroid classifier (recommended)

Collect ~10 readings per color after calibration, compute centroid vectors of (r,g,b) (normalized) or (h,s,v) for each color and store them. To classify, compute Euclidean distance to each centroid and pick the nearest — this handles boundary fuzziness much better than hard thresholds.

5) Arduino/ESP32 code snippet

Drop-in pseudocode (Arduino-style) — adapt to your TCS read code and storage:

struct Cal {
  float r_gain, g_gain, b_gain;
  uint16_t r_dark, g_dark, b_dark, c_dark;
};

Cal cal; // fill this per sensor at setup

void applyCalibration(uint16_t Rraw, uint16_t Graw, uint16_t Braw, uint16_t Craw,
                      float &Rout, float &Gout, float &Bout, float &Cout) {
  int R = max(0, (int)Rraw - (int)cal.r_dark);
  int G = max(0, (int)Graw - (int)cal.g_dark);
  int B = max(0, (int)Braw - (int)cal.b_dark);
  int C = max(1, (int)Craw - (int)cal.c_dark); // avoid div by zero

  float Rw = R * cal.r_gain;
  float Gw = G * cal.g_gain;
  float Bw = B * cal.b_gain;

  // normalize by clear to cancel brightness
  Rout = Rw / (float)C;
  Gout = Gw / (float)C;
  Bout = Bw / (float)C;
  Cout = C;
}

void rgbToHsv(float r, float g, float b, float &h, float &s, float &v) {
  // assume r,g,b in [0..1] - normalize first
  float maxv = max(r, max(g,b));
  float minv = min(r, min(g,b));
  v = maxv;
  float d = maxv - minv;
  s = (maxv == 0) ? 0 : d / maxv;
  if (d == 0) { h = 0; return; }
  if (maxv == r) h = fmod((g - b) / d, 6.0) * 60.0;
  else if (maxv == g) h = ((b - r) / d + 2.0) * 60.0;
  else h = ((r - g) / d + 4.0) * 60.0;
  if (h < 0) h += 360.0;
}

String classifyColor(float h, float s, float v) {
  // brown before orange
  if (h >= 15 && h <= 45 && v < 0.25) return "brown";
  if ((h >= 345 || h <= 15) && s > 0.35 && v > 0.15) return "red";
  if (h > 15 && h <= 45 && s > 0.35) return "orange";
  if (h > 45 && h <= 75 && s > 0.30) return "yellow";
  if (h > 75 && h <= 165) return "green";
  if (h > 165 && h <= 270) return "blue";
  if (h > 270 && h <= 315) return "purple";
  // pink: red-ish hue, lower sat but high value
  if ((h >= 345 || h <= 15) && s > 0.12 && s < 0.45 && v > 0.5) return "pink";
  return "unknown";
}

// usage in loop:
// read raw Rraw,Graw,Braw,Craw from sensor
float Rn, Gn, Bn, Cn;
applyCalibration(Rraw, Graw, Braw, Craw, Rn, Gn, Bn, Cn);

// scale Rn,Gn,Bn to 0..1 for hsv conversion (we can normalize)
float sum = Rn + Gn + Bn;
float r = Rn / sum;
float g = Gn / sum;
float b = Bn / sum;
float h, s, v;
rgbToHsv(r, g, b, h, s, v);
String color = classifyColor(h, s, v);
Serial.println(color);

6) Practical tips / pitfalls

Gate on C (clear): if clear < threshold the sample is too dark — treat as “no reading” or indicate low confidence.

Control lighting if possible: consistent illumination (or using the sensor’s own LED for each read) yields much better classification. If you can briefly strobe the sensor LED for readings, you remove a lot of ambient variation.

Collect per-sensor training data if you want reliable performance — save centroids or train a tiny k-NN (k=1 or 3) with ~10 samples per color. This is lightweight and robust.

Priority and fallback: check special cases first (brown test, pink test) before general hue buckets to reduce misclassification.

Store calibration: save per-sensor gains and dark offsets so you don’t re-calibrate every boot.