import { addMinutes, subHours } from 'date-fns';
import { ActuatorKey, ActuatorState, DashboardState, SensorKey, SensorPoint, SensorSeries } from './types';

const SENSOR_KEYS: SensorKey[] = ['light', 'airHumidity', 'soilMoisture', 'airTemp'];

const clamp = (v: number, min: number, max: number) => Math.max(min, Math.min(max, v));

function generateInitialSeries(): SensorSeries {
	const now = Date.now();
	const start = subHours(now, 24).getTime();
	const pointsPerMinute = 1; // per minute resolution
	const totalPoints = 24 * 60 * pointsPerMinute;

	const series: Partial<SensorSeries> = {};

	SENSOR_KEYS.forEach((key) => {
		const arr: SensorPoint[] = [];
		for (let i = 0; i < totalPoints; i++) {
			const ts = addMinutes(start, i).getTime();
			// baselines and noise per sensor
			let value = 0;
			if (key === 'light') {
				// Simulate day/night cycle 0-100 klux
				const hour = new Date(ts).getHours();
				const daylight = Math.max(0, Math.sin(((hour - 6) / 12) * Math.PI));
				value = clamp(daylight * 90 + randn(0, 6), 0, 100);
			} else if (key === 'airHumidity') {
				value = clamp(65 + Math.sin(i / 200) * 10 + randn(0, 3), 30, 100);
			} else if (key === 'soilMoisture') {
				value = clamp(45 + Math.sin(i / 300) * 8 + randn(0, 4), 10, 90);
			} else if (key === 'airTemp') {
				value = clamp(26 + Math.sin(i / 180) * 4 + randn(0, 0.8), 10, 45);
			}
			arr.push({ timestamp: ts, value: Number(value.toFixed(2)) });
		}
		(series as SensorSeries)[key] = arr;
	});
	return series as SensorSeries;
}

function randn(mu = 0, sigma = 1) {
	let u = 0, v = 0;
	while (u === 0) u = Math.random();
	while (v === 0) v = Math.random();
	return mu + sigma * Math.sqrt(-2.0 * Math.log(u)) * Math.cos(2.0 * Math.PI * v);
}

export function createInitialDashboardState(): DashboardState {
	const series = generateInitialSeries();
	const lastIdx = series.light.length - 1;
	const current = {
		light: series.light[lastIdx].value,
		airHumidity: series.airHumidity[lastIdx].value,
		soilMoisture: series.soilMoisture[lastIdx].value,
		airTemp: series.airTemp[lastIdx].value
	};
	const actuators: Record<ActuatorKey, ActuatorState> = {
		lamp: { key: 'lamp', name: 'Lampu', isOn: false },
		fan: { key: 'fan', name: 'Kipas', isOn: false },
		pump: { key: 'pump', name: 'Pompa', isOn: false }
	};
	return {
		series,
		current,
		mode: 'auto',
		actuators
	};
}

export function advanceOneMinute(state: DashboardState): DashboardState {
	const next: DashboardState = { ...state, series: { ...state.series }, current: { ...state.current }, actuators: { ...state.actuators } };
	const now = Date.now();
	(['light', 'airHumidity', 'soilMoisture', 'airTemp'] as SensorKey[]).forEach((key) => {
		const arr = next.series[key].slice(1); // drop oldest
		let base = next.current[key];
		// add small random walk
		if (key === 'light') {
			const hour = new Date(now).getHours();
			const daylight = Math.max(0, Math.sin(((hour - 6) / 12) * Math.PI));
			base = clamp(daylight * 90 + randn(0, 6), 0, 100);
		} else if (key === 'airHumidity') {
			base = clamp(base + randn(0, 1.5), 30, 100);
		} else if (key === 'soilMoisture') {
			base = clamp(base + randn(0, 1.8), 10, 90);
		} else if (key === 'airTemp') {
			base = clamp(base + randn(0, 0.4), 10, 45);
		}
		const point: SensorPoint = { timestamp: now, value: Number(base.toFixed(2)) };
		arr.push(point);
		next.series[key] = arr;
		next.current[key] = point.value;
	});
	// simple auto-control heuristic (placeholder for real backend rules)
	if (next.mode === 'auto') {
		next.actuators.lamp.isOn = next.current.light < 20;
		next.actuators.fan.isOn = next.current.airTemp > 30 || next.current.airHumidity > 80;
		next.actuators.pump.isOn = next.current.soilMoisture < 30;
	}
	return next;
}

export const SENSOR_META: Record<SensorKey, { label: string; unit: string; color: string }> = {
	light: { label: 'Intensitas Cahaya', unit: 'klux', color: '#fbbf24' },
	airHumidity: { label: 'Kelembapan Udara', unit: '%', color: '#60a5fa' },
	soilMoisture: { label: 'Kelembapan Tanah', unit: '%', color: '#34d399' },
	airTemp: { label: 'Suhu Udara', unit: '°C', color: '#f472b6' }
};

