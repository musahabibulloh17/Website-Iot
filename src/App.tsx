import { useEffect, useMemo, useRef, useState } from 'react';
import SensorCard from './components/SensorCard';
import TimeSeriesChart from './components/TimeSeriesChart';
import ActuatorPanel from './components/ActuatorPanel';
import { createInitialDashboardState, advanceOneMinute, SENSOR_META } from './data';
import { ActuatorKey, DashboardState, SensorKey } from './types';

export default function App() {
	const [state, setState] = useState<DashboardState>(() => createInitialDashboardState());
	const timerRef = useRef<number | null>(null);

	useEffect(() => {
		// Simulate realtime updates each minute (use shorter interval for demo)
		timerRef.current = window.setInterval(() => {
			setState(prev => advanceOneMinute(prev));
		}, 60_000);
		return () => {
			if (timerRef.current) window.clearInterval(timerRef.current);
		};
	}, []);

	// For demo convenience, also update every 5s so the chart moves during testing
	useEffect(() => {
		const fast = window.setInterval(() => {
			setState(prev => advanceOneMinute(prev));
		}, 5_000);
		return () => window.clearInterval(fast);
	}, []);

	const sensorOrder: SensorKey[] = ['light', 'airHumidity', 'soilMoisture', 'airTemp'];

	const chartGroups = useMemo(() => {
		return [
			{
				title: 'Grafik Realtime - Intensitas Cahaya',
				series: [{ key: 'light' as SensorKey, points: state.series.light }]
			},
			{
				title: 'Grafik Realtime - Kelembapan Udara',
				series: [{ key: 'airHumidity' as SensorKey, points: state.series.airHumidity }]
			},
			{
				title: 'Grafik Realtime - Kelembapan Tanah',
				series: [{ key: 'soilMoisture' as SensorKey, points: state.series.soilMoisture }]
			},
			{
				title: 'Grafik Realtime - Suhu Udara',
				series: [{ key: 'airTemp' as SensorKey, points: state.series.airTemp }]
			}
		];
	}, [state.series]);

	const setMode = (mode: 'auto' | 'manual') => {
		setState(prev => ({ ...prev, mode }));
	};
	const setActuator = (key: ActuatorKey, on: boolean) => {
		setState(prev => ({ ...prev, actuators: { ...prev.actuators, [key]: { ...prev.actuators[key], isOn: on } } }));
		// TODO: Integrate API call to backend here
	};
	const setAllActuators = (on: boolean) => {
		const next = { ...state.actuators };
		(['lamp', 'fan', 'pump'] as ActuatorKey[]).forEach(k => next[k] = { ...next[k], isOn: on });
		setState(prev => ({ ...prev, actuators: next }));
		// TODO: Integrate API call to backend here
	};

	return (
		<div className="container">
			<div style={{ display: 'flex', alignItems: 'baseline', gap: 10 }}>
				<div className="title">IoT Realtime Dashboard</div>
				<div className="subtitle">Pemantauan cahaya, kelembapan udara, kelembapan tanah, dan suhu</div>
			</div>

			<div className="grid" style={{ marginBottom: 12 }}>
				{sensorOrder.map((k) => (
					<SensorCard key={k} sensor={k} value={state.current[k]} />
				))}
			</div>

			<div className="grid" style={{ marginBottom: 12 }}>
				{chartGroups.map(group => (
					<TimeSeriesChart key={group.title} title={group.title} series={group.series} />
				))}
			</div>

			<ActuatorPanel
				mode={state.mode}
				onModeChange={setMode}
				actuators={state.actuators}
				onToggle={setActuator}
				onAll={setAllActuators}
			/>

			<div className="subtitle" style={{ marginTop: 18 }}>
				Catatan: Data realtime disimulasikan. Hubungkan ke WebSocket/REST untuk produksi.
			</div>
		</div>
	);
}

