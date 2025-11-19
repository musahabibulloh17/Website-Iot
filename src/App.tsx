import { useEffect, useMemo, useRef, useState } from 'react';
import SensorCard from './components/SensorCard';
import TimeSeriesChart from './components/TimeSeriesChart';
import ActuatorPanel from './components/ActuatorPanel';
import ThemeToggle from './components/ThemeToggle';
import { createInitialDashboardState, advanceOneMinute } from './data';
import { ActuatorKey, DashboardState, SensorKey } from './types';
import { useTheme } from './hooks/useTheme';
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
	const { theme, toggleTheme } = useTheme();

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
		<>
			<ThemeToggle theme={theme} onToggle={toggleTheme} />
			<div className="container">
			<div style={{ marginBottom: 32 }}>
				<div style={{ display: 'flex', alignItems: 'center', gap: 12, marginBottom: 8 }}>
					<div className="title" style={{ margin: 0 }}>🌱 IoT Dashboard</div>
					{USE_FIREBASE && (
						<div style={{ 
							fontSize: 11, 
							fontWeight: 600, 
							color: 'var(--ok)',
							display: 'flex',
							alignItems: 'center',
							gap: 6,
							padding: '4px 8px',
							background: 'rgba(16, 185, 129, 0.1)',
							border: '1px solid rgba(16, 185, 129, 0.3)',
							borderRadius: 6
						}}>
							<span style={{ width: 6, height: 6, background: 'var(--ok)', borderRadius: '50%', display: 'inline-block' }}></span>
							Connected
						</div>
					)}
					{!USE_FIREBASE && (
						<div style={{ 
							fontSize: 11, 
							fontWeight: 600, 
							color: 'var(--warning)',
							display: 'flex',
							alignItems: 'center',
							gap: 6,
							padding: '4px 8px',
							background: 'rgba(234, 179, 8, 0.1)',
							border: '1px solid rgba(234, 179, 8, 0.3)',
							borderRadius: 6
						}}>
							<span style={{ width: 6, height: 6, background: 'var(--warning)', borderRadius: '50%', display: 'inline-block' }}></span>
							Demo Mode
						</div>
					)}
				</div>
				<div style={{ fontSize: 13, color: 'var(--muted)' }}>
					Pemantauan real-time cahaya, kelembapan udara, kelembapan tanah, dan suhu
				</div>
			</div>

			<div className="grid" style={{ marginBottom: 24 }}>
				{sensorOrder.map((k) => (
					<SensorCard key={k} sensor={k} value={state.current[k]} />
				))}
			</div>

			<div className="grid" style={{ marginBottom: 24 }}>
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

			<div style={{ marginTop: 24, padding: '12px 16px', background: 'rgba(56, 189, 248, 0.05)', border: '1px solid rgba(56, 189, 248, 0.2)', borderRadius: 10, fontSize: 12, color: 'var(--muted)' }}>
				{USE_FIREBASE 
					? '✅ Data real-time dari Firebase Realtime Database'
					: '📊 Demo Mode • Ubah VITE_MODE=firebase untuk production'
				}
			</div>
			</div>
		</>
	);
}

