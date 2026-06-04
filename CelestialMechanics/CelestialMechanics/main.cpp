#define _USE_MATH_DEFINES

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <cmath>


using namespace std;

//06MI3400853 (3) -> Table 8.10.3 на страница 28, както и Table 8.8.3 на страници 19-20 за относителните маси от статията Orbital Elements ...(OE)

const double DEG2RAD = M_PI / 180.0;

struct Planet {
	string name;
	double a, a_delta, a_calc;   // semi-major axis
	double e, e_delta, e_calc;   // eccentricity
	double I, I_delta, I_calc;   // inclination
	double L, L_delta, L_calc;   // mean longitude
	double W, W_delta, W_calc;   // longitude of perihelion
	double O, O_delta, O_calc;   // longitude of asc. node
	double mass;                 // planetary mass M_sun/M_planet [mass^-1]
	double n;
	double X, Y, Z;
	double dX, dY, dZ;

	// Delaunay elements  (L,G,Th are momenta; l,g,th are conjugate coordinates)
	double del_L;    // L     = mu * sqrt(gamma * a)
	double del_G;    // G     = L  * sqrt(1 - e^2)
	double del_T;    // Theta = G  * cos(i)
	double del_l;    // l     = mean anomaly M  (rad)
	double del_g;    // g     = omega = W - O   (argument of perihelion, rad)
	double del_t;    // theta = Omega = O       (longitude of asc. node, rad)

	// Poincare first kind  (Lambda,LmG,GmT are momenta; lambda,-varpi,-theta coords)
	double poi1_Lambda;   // Lambda =  L
	double poi1_LmG;      // L - G
	double poi1_GmT;      // G - Theta
	double poi1_lambda;   // lambda  = l + g + theta  (mean longitude, rad)
	double poi1_mvarpi;   // -varpi  = -(g + theta)
	double poi1_mtheta;   // -theta

	// Poincare second kind  (Lambda,xi,p are momenta; lambda,eta,q coords)
	double poi2_Lambda;   // Lambda =  L
	double poi2_lambda;   // lambda  = l + g + theta
	double poi2_xi;       // xi  =  sqrt(2*(L-G)) * cos(g+theta)
	double poi2_eta;      // eta = -sqrt(2*(L-G)) * sin(g+theta)
	double poi2_p;        // p   =  sqrt(2*(G-Theta)) * cos(theta)
	double poi2_q;        // q   = -sqrt(2*(G-Theta)) * sin(theta)
};

bool isLeap(int year) {
	return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

//--- CSV helpers ---
string trim(const string& s) {
	size_t a = s.find_first_not_of(" \t\r\n");
	size_t b = s.find_last_not_of(" \t\r\n");
	return (a == string::npos) ? "" : s.substr(a, b - a + 1);
}

// Read one comma-separated token from a stringstream
string nextToken(stringstream& ss) {
	string tok;
	getline(ss, tok, ',');
	return trim(tok);
}
//-------------

double calculateEccentricAnomaly(double M, double e, int depth = 10) {
	if (depth == 0) return M;
	return M + e * sin(calculateEccentricAnomaly(M, e, depth - 1));
}

void writeHTML(const vector<Planet>& planets, int day, int month, int year)
{
	ofstream f("visualization.html");
	if (!f) { cerr << "Cannot write visualization.html\n"; return; }

	auto color = [](const string& n) -> string {
		if (n == "Mercury")                    return "#b5b5b5";
		if (n == "Venus")                      return "#e8c97a";
		if (n == "EM Bary" || n == "Earth")    return "#4fa3e0";
		if (n == "Mars")                       return "#c1440e";
		if (n == "Jupiter")                    return "#c88b3a";
		if (n == "Saturn")                     return "#e4d191";
		if (n == "Uranus")                     return "#7de8e8";
		if (n == "Neptune")                    return "#4b70dd";
		return "#aaaaaa";
		};

	f << "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>\n";
	f << "<title>Solar system - " << day << "/" << month << "/" << year << "</title>\n";
	f << "<style>\n";
	f << "* { box-sizing: border-box; margin: 0; padding: 0; }\n";
	f << "body { background: #0a0a14; color: #ccc; font-family: sans-serif; padding: 12px; }\n";
	f << "canvas { display: block; width: 100%; height: 540px; border-radius: 8px;\n";
	f << "         border: 1px solid #333; cursor: grab; }\n";
	f << "canvas:active { cursor: grabbing; }\n";
	f << "h1 { font-size: 15px; font-weight: 500; color: #eee; margin-bottom: 8px; }\n";
	f << ".hint { font-size: 12px; color: #555; text-align: center; margin: 5px 0 10px; }\n";
	f << ".controls { display: flex; gap: 10px; align-items: center; flex-wrap: wrap; margin-bottom: 10px; }\n";
	f << ".controls label { font-size: 13px; display: flex; align-items: center; gap: 5px; cursor: pointer; }\n";
	f << "button { font-size: 12px; padding: 4px 10px; background: #1a1a2e; color: #ccc;\n";
	f << "         border: 1px solid #444; border-radius: 4px; cursor: pointer; }\n";
	f << "button:hover { background: #252540; }\n";
	f << "table { width: 100%; border-collapse: collapse; font-size: 13px; margin-top: 10px; }\n";
	f << "th { text-align: left; padding: 5px 8px; color: #888; border-bottom: 1px solid #333; }\n";
	f << "td { padding: 4px 8px; border-bottom: 1px solid #1a1a2a; font-family: monospace; font-size: 12px; }\n";
	f << ".dot { display: inline-block; width: 9px; height: 9px; border-radius: 50%; margin-right: 6px; vertical-align: middle; }\n";
	f << "</style></head><body>\n";
	f << "<h1>Solar system &mdash; " << day << "/" << month << "/" << year << "</h1>\n";
	f << "<canvas id='c'></canvas>\n";
	f << "<p class='hint'>Drag to rotate &nbsp;&middot;&nbsp; scroll to zoom</p>\n";
	f << "<div class='controls'>\n";
	f << "  <button onclick='resetView()'>Reset view</button>\n";
	f << "  <label><input type='checkbox' id='chkLbl' checked onchange='render()'> Labels</label>\n";
	f << "  <label><input type='checkbox' id='chkOrb' checked onchange='render()'> Orbits</label>\n";
	f << "  <label style='margin-left:auto'>Scale&nbsp;<input type='range' id='sc' min='2' max='400' value='25' style='width:90px' oninput='render()'></label>\n";
	f << "</div>\n";

	f << "<table><thead><tr>"
		<< "<th>Planet</th><th>X (AU)</th><th>Y (AU)</th><th>Z (AU)</th>"
		<< "<th>|r| (AU)</th><th>mass (M&#x2609;/M)</th><th>n </th>"
		<< "</tr></thead><tbody>\n";
	for (const Planet& p : planets) {
		double r = sqrt(p.X * p.X + p.Y * p.Y + p.Z * p.Z);
		f << fixed << setprecision(6);
		f << "<tr><td><span class='dot' style='background:" << color(p.name) << "'></span>"
			<< p.name << "</td>"
			<< "<td>" << p.X << "</td><td>" << p.Y << "</td><td>" << p.Z << "</td>"
			<< "<td>" << r << "</td>"
			<< setprecision(2)
			<< "<td>" << p.mass << "</td>"
			<< setprecision(6)
			<< "<td>" << p.n << "</td></tr>\n";
	}
	f << "</tbody></table>\n";

	f << "<script>\nconst planets = [\n";
	for (const Planet& p : planets) {
		f << fixed << setprecision(8);
		// w and O_rad and I_rad are needed in JS to reconstruct the orbit plane
		double w_rad = (p.W_calc - p.O_calc) * M_PI / 180.0;
		double O_rad = p.O_calc * M_PI / 180.0;
		double I_rad = p.I_calc * M_PI / 180.0;
		f << "  {name:'" << p.name << "'"
			<< ", x:" << p.X << ", y:" << p.Y << ", z:" << p.Z
			<< ", a:" << p.a_calc << ", e:" << p.e_calc
			<< ", w:" << w_rad << ", O:" << O_rad << ", I:" << I_rad
			<< ", col:'" << color(p.name) << "'},\n";
	}
	f << "];\n";

	f << R"JS(
		const canvas = document.getElementById('c');
		const ctx    = canvas.getContext('2d');
 
		// ---------- matrix helpers ----------
		// Rotation is stored as a flat 9-element row-major 3x3 matrix.
		// Every drag event premultiplies in WORLD space:
		//   R = Rx(dy) * Ry(dx) * R
		// Left/right drag is always a pure Y-axis yaw.
		// Up/down   drag is always a pure X-axis pitch.
		// Because we work in world space, axes never drift -> no roll.
 
		function mat3Identity() { return [1,0,0, 0,1,0, 0,0,1]; }
 
		function mat3Mul(A, B) {
		  const C = new Array(9);
		  for (let r = 0; r < 3; r++)
			for (let c = 0; c < 3; c++)
			  C[r*3+c] = A[r*3]*B[c] + A[r*3+1]*B[3+c] + A[r*3+2]*B[6+c];
		  return C;
		}
 
		function mat3RotY(a) {
		  const c = Math.cos(a), s = Math.sin(a);
		  return [c,0,s, 0,1,0, -s,0,c];
		}
 
		function mat3RotX(a) {
		  const c = Math.cos(a), s = Math.sin(a);
		  return [1,0,0, 0,c,-s, 0,s,c];
		}
 
		function applyR(M, x, y, z) {
		  return {
			x: M[0]*x + M[1]*y + M[2]*z,
			y: M[3]*x + M[4]*y + M[5]*z,
			z: M[6]*x + M[7]*y + M[8]*z
		  };
		}
		// ------------------------------------
 
		// Compute 360 3-D points along a Keplerian ellipse.
		// Uses the same rotation as the position formula in main.cpp:
		//   x = R11*xkep + R12*ykep,  y = R21*xkep + R22*ykep,  z = R31*xkep + R32*ykep
		// where xkep = a*(cosE - e), ykep = a*sqrt(1-e^2)*sinE
		function orbitPoints(a, e, w, O, I) {
		  const pts = [];
		  const cosw = Math.cos(w), sinw = Math.sin(w);
		  const cosO = Math.cos(O), sinO = Math.sin(O);
		  const cosI = Math.cos(I), sinI = Math.sin(I);
		  const b = a * Math.sqrt(1 - e * e);
		  const N = 180;
		  for (let k = 0; k <= N; k++) {
			const E = (2 * Math.PI * k) / N;
			const xk = a * (Math.cos(E) - e);
			const yk = b * Math.sin(E);
			const px = (cosw*cosO - sinw*sinO*cosI)*xk + (-sinw*cosO - cosw*sinO*cosI)*yk;
			const py = (cosw*sinO + sinw*cosO*cosI)*xk + (-sinw*sinO + cosw*cosO*cosI)*yk;
			const pz = (sinw*sinI)*xk + (cosw*sinI)*yk;
			pts.push([px, py, pz]);
		  }
		  return pts;
		}
 
		let R = mat3Mul(mat3RotX(0.4), mat3RotY(0.5));   // initial view: slight top-down tilt
		let drag = false, lx = 0, ly = 0;
 
		function proj(x, y, z, cx, cy, sc) {
		  const v = applyR(R, x, y, z);
		  return { sx: cx + v.x * sc, sy: cy - v.y * sc, depth: v.z };
		}
 
		function render() {
		  const dpr = window.devicePixelRatio || 1;
		  const W = canvas.clientWidth, H = canvas.clientHeight;
		  canvas.width = W*dpr; canvas.height = H*dpr;
		  ctx.scale(dpr, dpr);
		  const sc  = +document.getElementById('sc').value;
		  const lbl = document.getElementById('chkLbl').checked;
		  const orb = document.getElementById('chkOrb').checked;
		  const cx = W/2, cy = H/2;
 
		  ctx.fillStyle = '#0a0a14'; ctx.fillRect(0,0,W,H);
 
		  // Draw orbits first (behind everything)
		  if (orb) {
			planets.forEach(p => {
			  const pts = orbitPoints(p.a, p.e, p.w, p.O, p.I);
			  ctx.strokeStyle = p.col + '55'; ctx.lineWidth = 0.7; ctx.setLineDash([]);
			  ctx.beginPath();
			  pts.forEach(([x,y,z], i) => {
				const q = proj(x, y, z, cx, cy, sc);
				i === 0 ? ctx.moveTo(q.sx, q.sy) : ctx.lineTo(q.sx, q.sy);
			  });
			  ctx.closePath(); ctx.stroke();
			});
		  }
 
		  const sun = proj(0,0,0,cx,cy,sc);
		  ctx.beginPath(); ctx.arc(sun.sx,sun.sy,7,0,Math.PI*2);
		  ctx.fillStyle = '#ffd966'; ctx.fill();
		  if (lbl) {
			ctx.fillStyle='#ffd966'; ctx.font='500 12px sans-serif';
			ctx.fillText('Sun', sun.sx+9, sun.sy+4);
		  }
 
		  const projected = planets.map(p => ({
			...p,
			pt: proj(p.x, p.y, p.z, cx, cy, sc)
		  })).sort((a,b) => b.pt.depth - a.pt.depth);
 
		  projected.forEach(p => {
			const r  = Math.sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
			const dr = Math.max(3, Math.min(7, 2 + r*0.3));
			const s0 = proj(0,0,0,cx,cy,sc);
 
			ctx.strokeStyle = p.col+'33'; ctx.lineWidth=0.5; ctx.setLineDash([]);
			ctx.beginPath(); ctx.moveTo(s0.sx,s0.sy); ctx.lineTo(p.pt.sx,p.pt.sy); ctx.stroke();
 
			ctx.beginPath(); ctx.arc(p.pt.sx,p.pt.sy,dr,0,Math.PI*2);
			ctx.fillStyle=p.col; ctx.fill();
 
			if (lbl) {
			  ctx.fillStyle=p.col; ctx.font='500 12px sans-serif';
			  ctx.fillText(p.name, p.pt.sx+dr+3, p.pt.sy+4);
			  ctx.fillStyle='rgba(200,200,200,0.45)'; ctx.font='11px monospace';
			  ctx.fillText(r.toFixed(3)+' AU', p.pt.sx+dr+3, p.pt.sy+16);
			}
		  });
		}
 
		function resetView() {
		  R = mat3Mul(mat3RotX(0.4), mat3RotY(0.5));
		  document.getElementById('sc').value = 25;
		  render();
		}
 
		canvas.addEventListener('mousedown', e => { drag=true; lx=e.clientX; ly=e.clientY; });
		window.addEventListener('mouseup',   ()  => { drag=false; });
		window.addEventListener('mousemove', e  => {
		  if (!drag) return;
		  const dx = (e.clientX-lx)*0.008;   // left/right  -> world Y yaw
		  const dy = (e.clientY-ly)*0.008;   // up/down     -> world X pitch
		  lx=e.clientX; ly=e.clientY;
		  // Premultiply in world space: axes stay fixed regardless of current orientation
		  R = mat3Mul(mat3Mul(mat3RotX(dy), mat3RotY(dx)), R);
		  render();
		});
		canvas.addEventListener('wheel', e => {
		  e.preventDefault();
		  const s = document.getElementById('sc');
		  const current = +s.value;
		  const delta = e.deltaY * 0.04 * (current / 25);   // proportional speed: faster when zoomed in
		  s.value = Math.max(2, Math.min(400, current - delta));
		  render();
		}, {passive:false});
 
		window.addEventListener('resize', render);
		render();
		</script>
		</body></html>
	)JS";

	f.close();
	cout << "\nVisualization written to visualization.html\n";
}

int main()
{
	int day, month, year;

	cout << "Enter Day:" << endl;
	cin >> day;

	cout << "Enter Month:" << endl;
	cin >> month;

	cout << "Enter Year:" << endl;
	cin >> year;

	int daysInMonth[] = { 31, isLeap(year) ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int totalDays = 0;

	//add years
	for (int y = 2000; y < year; y++) {
		totalDays += isLeap(y) ? 366 : 365;
	}

	// add months
	for (int m = 0; m < month - 1; m++) {
		totalDays += daysInMonth[m];
	}

	//add days
	totalDays += day;

	double t = totalDays / (365.25 * 100.0);

	cout << fixed << setprecision(6);
	cout << "Days since 1.1.2000: " << totalDays << endl;
	cout << "Centuries since 1.1.2000: " << t << endl;


	vector<Planet> planets;

	ifstream file("../table.txt");
	if (!file) { cerr << "Cannot open table.txt\n"; return 1; }

	string line;
	getline(file, line); // skip header row

	while (getline(file, line)) {
		if (trim(line).empty()) continue;
		stringstream ss(line);
		Planet p;
		p.name = nextToken(ss);
		p.a = stod(nextToken(ss));
		p.a_delta = stod(nextToken(ss));
		p.e = stod(nextToken(ss));
		p.e_delta = stod(nextToken(ss));
		p.I = stod(nextToken(ss));
		p.I_delta = stod(nextToken(ss));
		p.L = stod(nextToken(ss));
		p.L_delta = stod(nextToken(ss));
		p.W = stod(nextToken(ss));
		p.W_delta = stod(nextToken(ss));
		p.O = stod(nextToken(ss));
		p.O_delta = stod(nextToken(ss));
		p.mass = stod(nextToken(ss));
		planets.push_back(p);
	}

	for (Planet& p : planets)
	{
		p.a_calc = (p.a + p.a_delta * t);
		p.e_calc = (p.e + p.e_delta * t);
		p.I_calc = (p.I + p.I_delta * t);
		p.L_calc = (p.L + p.L_delta * t);
		p.W_calc = (p.W + p.W_delta * t);
		p.O_calc = (p.O + p.O_delta * t);

		double M  = (p.L_calc - p.W_calc) * DEG2RAD; // mean anomaly in rad (0 <= M < 2pi)
		M = fmod(M, 2.0 * M_PI);
		if (M < 0) M += 2.0 * M_PI;

		double E = calculateEccentricAnomaly(M, p.e_calc);

		double w = (p.W_calc - p.O_calc) * DEG2RAD; //perihelion argument

		// covert to radians
		double O_rad = p.O_calc * DEG2RAD;
		double I_rad = p.I_calc * DEG2RAD;

		// Kepler movement
		double X_kep = p.a_calc * (cos(E) - p.e_calc);
		double Y_kep = p.a_calc * (sqrt(1 - (p.e_calc * p.e_calc)) * sin(E));
		double Z_kep = 0.0;

		// Decart coordinates
		p.X = (cos(w) * cos(O_rad) - sin(w) * sin(O_rad) * cos(I_rad)) * X_kep + (-sin(w) * cos(O_rad) - cos(w) * sin(O_rad) * cos(I_rad)) * Y_kep;
		p.Y = (cos(w) * sin(O_rad) + sin(w) * cos(O_rad) * cos(I_rad)) * X_kep + (-sin(w) * sin(O_rad) + cos(w) * cos(O_rad) * cos(I_rad)) * Y_kep;
		p.Z = (sin(w) * sin(I_rad)) * X_kep + (cos(w) * sin(I_rad)) * Y_kep;

		double m = 1.0 / p.mass;
		p.n = sqrt((1 + m) / (pow(p.a_calc, 3))); // mean movement

		// Kepler speed
		double dX_kep = -(p.a_calc * p.n * sin(E)) / (1 - (p.e_calc * cos(E)));
		double dY_kep = (p.a_calc * p.n * sqrt(1 - (p.e_calc * p.e_calc)) * cos(E)) / (1 - (p.e_calc * cos(E)));
		double dZ_kep = 0.0;

		// Speed vector
		p.dX = (((cos(w) * cos(O_rad)) - (sin(w) * sin(O_rad) * cos(I_rad))) * dX_kep) + (((-sin(w) * cos(O_rad)) - (-cos(w) * sin(O_rad) * cos(I_rad))) * dY_kep);
		p.dY = (((cos(w) * sin(O_rad)) + (sin(w) * cos(O_rad) * cos(I_rad))) * dX_kep) + (((-sin(w) * sin(O_rad)) + (-cos(w) * cos(O_rad) * cos(I_rad))) * dY_kep);
		p.dZ = ((sin(w) * sin(I_rad)) * dX_kep) + ((cos(w) * sin(I_rad)) * dY_kep);

		// Delaunay
		double gamma = 1.0 + m; // (gravitational correction)

		p.del_L = m * sqrt(gamma * p.a_calc);
		p.del_G = p.del_L * sqrt(1.0 - (p.e_calc * p.e_calc));
		p.del_T = p.del_G * cos(I_rad);          // I_rad already in radians
		p.del_l = M;                             // mean anomaly (rad), computed above
		p.del_g = w;                             // omega = W - O  (rad), computed above
		p.del_t = O_rad;                         // Omega = O  (rad), computed above

		// Poincare first kind
		// Momenta:  Lambda, L-G, G-Theta
		// Coords:   lambda = l+g+theta,  -varpi = -(g+theta),  -theta

		p.poi1_Lambda = p.del_L;
		p.poi1_LmG = p.del_L - p.del_G;
		p.poi1_GmT = p.del_G - p.del_T;
		p.poi1_lambda = p.del_l + p.del_g + p.del_t;
		p.poi1_mvarpi = -p.del_g - p.del_t;
		p.poi1_mtheta = -p.del_t;


		// Poincare second kind
		// xi  =  sqrt(2*(L-G)) * cos(g+theta)
		// eta = -sqrt(2*(L-G)) * sin(g+theta)
		// p  =  sqrt(2*(G-Theta)) * cos(theta)
		// q   = -sqrt(2*(G-Theta)) * sin(theta)

		p.poi2_Lambda = p.del_L;
		p.poi2_lambda = p.del_l + p.del_g + p.del_t;
		p.poi2_xi = sqrt(2.0 * (p.del_L - p.del_G)) * cos(p.del_g + p.del_t);
		p.poi2_eta = -sqrt(2.0 * (p.del_L - p.del_G)) * sin(p.del_g + p.del_t);
		p.poi2_p = sqrt(2.0 * (p.del_G - p.del_T)) * cos(p.del_t);
		p.poi2_q = -sqrt(2.0 * (p.del_G - p.del_T)) * sin(p.del_t);
	}

	cout << string(115, '=') << "\n";
	cout << left << setw(12) << "Planet"
		<< right
		<< setw(10) << "a [AU]"
		<< setw(12) << "e [rad]"
		<< setw(12) << "I [deg]"
		<< setw(12) << "L [deg]"
		<< setw(12) << "W [deg]"
		<< setw(12) << "O [deg]"
		<< setw(18) << "m [mass^-1]"
		<< setw(12) << "n"
		<< "\n";
	cout << string(115, '-') << "\n";

	for (Planet& p : planets)
	{
		cout << left << setw(10) << p.name
			<< right << setprecision(6)
			<< setw(12) << p.a_calc
			<< setw(12) << p.e_calc
			<< setw(12) << p.I_calc
			<< setw(12) << p.L_calc
			<< setw(12) << p.W_calc
			<< setw(12) << p.O_calc
			<< setprecision(2) << setw(18) << p.mass
			<< setprecision(6) << setw(12) << p.n
			<< endl;
	}

	cout << endl << endl;

	cout << string(110, '=') << "\n";
	cout << left << setw(12) << "Planet"
		<< right
		<< setw(10) << "X"
		<< setw(12) << "Y"
		<< setw(12) << "Z"
		<< setw(12) << "|r|"
		<< setw(12) << "dX"
		<< setw(12) << "dY"
		<< setw(12) << "dZ"
		<< setw(12) << "|v|"
		<< endl;
	cout << string(110, '-') << "\n";

	for (Planet& p : planets)
	{
		cout << left << setw(10) << p.name
			<< right << setprecision(6)
			<< setw(12) << p.X
			<< setw(12) << p.Y
			<< setw(12) << p.Z
			<< setw(12) << sqrt((p.X * p.X) + (p.Y * p.Y) + (p.Z * p.Z))
			<< setw(12) << p.dX
			<< setw(12) << p.dY
			<< setw(12) << p.dZ
			<< setw(12) << sqrt((p.dX * p.dX) + (p.dY * p.dY) + (p.dZ * p.dZ))
			<< endl;
	}

	cout << endl << endl;

	// ---- Delaunay table ----
	const int SW = 18;  // column width for scientific notation (e.g. " 1.234567e-06")
	cout << string(10 + SW * 6, '=') << "\n";
	cout << "Delaunay elements\n";
	cout << string(10 + SW * 6, '-') << "\n";
	cout << left << setw(10) << "Planet"
		<< right << setw(SW) << "L"
		<< setw(SW) << "G"
		<< setw(SW) << "THETA"
		<< setw(SW) << "l (rad)"
		<< setw(SW) << "g (rad)"
		<< setw(SW) << "theta (rad)" << "\n";
	cout << string(10 + SW * 6, '-') << "\n";
	for (const Planet& p : planets) {
		cout << left << setw(10) << p.name
			<< right << scientific << setprecision(6)
			<< setw(SW) << p.del_L
			<< setw(SW) << p.del_G
			<< setw(SW) << p.del_T
			<< setw(SW) << p.del_l
			<< setw(SW) << p.del_g
			<< setw(SW) << p.del_t << "\n";
	}

	cout << endl << endl;

	// ---- Poincare first kind table ----
	cout << string(10 + SW * 6, '=') << "\n";
	cout << "Poincare elements - first kind\n";
	cout << string(10 + SW * 6, '-') << "\n";
	cout << left << setw(10) << "Planet"
		<< right << setw(SW) << "Lambda"
		<< setw(SW) << "L-G"
		<< setw(SW) << "G-Theta"
		<< setw(SW) << "lambda"
		<< setw(SW) << "-varpi"
		<< setw(SW) << "-theta" << "\n";
	cout << string(10 + SW * 6, '-') << "\n";
	for (const Planet& p : planets) {
		cout << left << setw(10) << p.name
			<< right << scientific << setprecision(6)
			<< setw(SW) << p.poi1_Lambda
			<< setw(SW) << p.poi1_LmG
			<< setw(SW) << p.poi1_GmT
			<< setw(SW) << p.poi1_lambda
			<< setw(SW) << p.poi1_mvarpi
			<< setw(SW) << p.poi1_mtheta << "\n";
	}

	cout << endl << endl;

	// ---- Poincare second kind table ----
	cout << string(10 + SW * 6, '=') << "\n";
	cout << "Poincare elements - second kind\n";
	cout << string(10 + SW * 6, '-') << "\n";
	cout << left << setw(10) << "Planet"
		<< right << setw(SW) << "Lambda"
		<< setw(SW) << "lambda"
		<< setw(SW) << "xi"
		<< setw(SW) << "eta"
		<< setw(SW) << "p"
		<< setw(SW) << "q" << "\n";
	cout << string(10 + SW * 6, '-') << "\n";
	for (const Planet& p : planets) {
		cout << left << setw(10) << p.name
			<< right << scientific << setprecision(6)
			<< setw(SW) << p.poi2_Lambda
			<< setw(SW) << p.poi2_lambda
			<< setw(SW) << p.poi2_xi
			<< setw(SW) << p.poi2_eta
			<< setw(SW) << p.poi2_p
			<< setw(SW) << p.poi2_q << "\n";
	}

	writeHTML(planets, day, month, year);

	return 0;
}
