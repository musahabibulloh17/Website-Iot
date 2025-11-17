import { useEffect, useMemo, useRef, useState } from 'react';
import SensorCard from './components/SensorCard';
import TimeSeriesChart from './components/TimeSeriesChart';
import ActuatorPanel from './components/ActuatorPanel';
import { createInitialDashboardState, advanceOneMinute } from './data';
import { ActuatorKey, DashboardState, SensorKey } from './types';
import { 
	subscribeToSensorCurrent,
	subscribeSensorTimeSeries,
	subscribeActuatorState,
	updateActuatorCommand,
	updateSystemMode,
	subscribeSystemMode
} from './firebase';

const USE_FIREBASE = import.meta.env.VITE_MODE === 'firebase';

export default function App() {
	const [state, setState] = useState<DashboardState>(() => createInitialDashboardState());
	const timerRef = useRef<number | null>(null);
	const unsubscribesRef = useRef<(() => void)[]>([]);

	// Subscribe to Firebase realtime data
	useEffect(() => {
		if (!USE_FIREBASE) {
			// Demo mode - use simulated data
			timerRef.current = window.setInterval(() => {
				setState(prev => advanceOneMinute(prev));
			}, 5_000);
			return () => {
				if (timerRef.current) window.clearInterval(timerRef.current);
			};
		}

		// Firebase mode - subscribe to real data
		const sensorKeys: SensorKey[] = ['light', 'airHumidity', 'soilMoisture', 'airTemp'];

		const handleError = (error: Error) => {
			console.error('Firebase subscription error:', error);
		};

		// Subscribe to sensor current values
		sensorKeys.forEach(key => {
			const unsubscribe = subscribeToSensorCurrent(
				key,
				(value) => {
					setState(prev => ({
						...prev,
						current: { ...prev.current, [key]: value }
					}));
				},
				handleError
			);
			unsubscribesRef.current.push(unsubscribe);
		});

		// Subscribe to sensor time series
		sensorKeys.forEach(key => {
			const unsubscribe = subscribeSensorTimeSeries(
				key,
				(points) => {
					setState(prev => ({
						...prev,
						series: {
							...prev.series,
							[key]: points
						}
					}));
				},
				handleError
			);
			unsubscribesRef.current.push(unsubscribe);
		});

		// Subscribe to actuator states
		(['lamp', 'fan', 'pump'] as ActuatorKey[]).forEach(key => {
			const unsubscribe = subscribeActuatorState(
				key,
				(isOn) => {
					setState(prev => ({
						...prev,
						actuators: {
							...prev.actuators,
							[key]: { ...prev.actuators[key], isOn }
						}
					}));
				},
				handleError
			);
			unsubscribesRef.current.push(unsubscribe);
		});

		// Subscribe to system mode
		const modeUnsubscribe = subscribeSystemMode(
			(mode) => {
				setState(prev => ({ ...prev, mode }));
			},
			handleError
		);
		unsubscribesRef.current.push(modeUnsubscribe);

		return () => {
			unsubscribesRef.current.forEach(unsub => unsub());
			unsubscribesRef.current = [];
		};
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

	const setMode = async (mode: 'auto' | 'manual') => {
		setState(prev => ({ ...prev, mode }));
		if (USE_FIREBASE) {
			try {
				await updateSystemMode(mode);
			} catch (error) {
				console.error('Failed to update mode:', error);
			}
		}
	};

	const setActuator = async (key: ActuatorKey, on: boolean) => {
		setState(prev => ({ ...prev, actuators: { ...prev.actuators, [key]: { ...prev.actuators[key], isOn: on } } }));
		if (USE_FIREBASE) {
			try {
				await updateActuatorCommand(key, on);
			} catch (error) {
				console.error('Failed to update actuator:', error);
			}
		}
	};

	const setAllActuators = async (on: boolean) => {
		const next = { ...state.actuators };
		(['lamp', 'fan', 'pump'] as ActuatorKey[]).forEach(k => next[k] = { ...next[k], isOn: on });
		setState(prev => ({ ...prev, actuators: next }));
		
		if (USE_FIREBASE) {
			try {
				await Promise.all([
					updateActuatorCommand('lamp', on),
					updateActuatorCommand('fan', on),
					updateActuatorCommand('pump', on)
				]);
			} catch (error) {
				console.error('Failed to update actuators:', error);
			}
		}
	};

	return (
		<div className="container">
			<div style={{ display: 'flex', alignItems: 'baseline', gap: 10 }}>
				<div className="title">IoT Realtime Dashboard</div>
				<div className="subtitle">Pemantauan cahaya, kelembapan udara, kelembapan tanah, dan suhu</div>
				{USE_FIREBASE && (
					<div className="badge" style={{ marginLeft: 'auto', background: '#22c55e', color: 'white', padding: '4px 8px', borderRadius: '4px', fontSize: '12px' }}>
						🔥 Connected to Firebase
					</div>
				)}
				{!USE_FIREBASE && (
					<div className="badge" style={{ marginLeft: 'auto', background: '#f59e0b', color: 'white', padding: '4px 8px', borderRadius: '4px', fontSize: '12px' }}>
						📊 Demo Mode
					</div>
				)}
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
				{USE_FIREBASE 
					? '✅ Data real-time dari Firebase Realtime Database'
					: '📊 Data disimulasikan. Ubah VITE_MODE=firebase untuk production'
				}
			</div>
		</div>
	);
}

