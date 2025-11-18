import { initializeApp } from 'firebase/app';
import { getDatabase, ref, onValue, set } from 'firebase/database';
import { SensorKey, ActuatorKey } from './types';

// Firebase Config dari environment variables
const firebaseConfig = {
	apiKey: import.meta.env.VITE_FIREBASE_API_KEY,
	authDomain: import.meta.env.VITE_FIREBASE_AUTH_DOMAIN,
	projectId: import.meta.env.VITE_FIREBASE_PROJECT_ID,
	storageBucket: import.meta.env.VITE_FIREBASE_STORAGE_BUCKET,
	messagingSenderId: import.meta.env.VITE_FIREBASE_MESSAGING_SENDER_ID,
	appId: import.meta.env.VITE_FIREBASE_APP_ID,
	databaseURL: import.meta.env.VITE_FIREBASE_DATABASE_URL,
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
export const database = getDatabase(app);

/**
 * Subscribe to current sensor readings
 */
export function subscribeToSensorCurrent(
	sensorKey: SensorKey,
	onUpdate: (value: number) => void,
	onError: (error: Error) => void
) {
	try {
		const sensorRef = ref(database, `sensors/${sensorKey}/current`);
		const unsubscribe = onValue(
			sensorRef,
			(snapshot) => {
				if (snapshot.exists()) {
					const data = snapshot.val() as any;
					onUpdate(data.value ?? 0);
				}
			},
			(error: any) => onError(error as Error)
		);
		return unsubscribe;
	} catch (error) {
		onError(error as Error);
		return () => {};
	}
}

/**
 * Subscribe to sensor time series data
 */
export function subscribeSensorTimeSeries(
	sensorKey: SensorKey,
	onUpdate: (points: Array<{ timestamp: number; value: number }>) => void,
	onError: (error: Error) => void
) {
	try {
		const seriesRef = ref(database, `sensors/${sensorKey}/series`);
		const unsubscribe = onValue(
			seriesRef,
			(snapshot) => {
				if (snapshot.exists()) {
					const data = snapshot.val() as any;
					const points = Array.isArray(data) ? data : Object.values(data || {});
					onUpdate(points as Array<{ timestamp: number; value: number }>);
				}
			},
			(error: any) => onError(error as Error)
		);
		return unsubscribe;
	} catch (error) {
		onError(error as Error);
		return () => {};
	}
}

/**
 * Subscribe to actuator state
 */
export function subscribeActuatorState(
	actuatorKey: ActuatorKey,
	onUpdate: (isOn: boolean) => void,
	onError: (error: Error) => void
) {
	try {
		const stateRef = ref(database, `actuators/${actuatorKey}/state`);
		const unsubscribe = onValue(
			stateRef,
			(snapshot) => {
				if (snapshot.exists()) {
					const data = snapshot.val() as any;
					onUpdate(data.isOn ?? false);
				}
			},
			(error: any) => onError(error as Error)
		);
		return unsubscribe;
	} catch (error) {
		onError(error as Error);
		return () => {};
	}
}

/**
 * Update actuator command
 */
export async function updateActuatorCommand(
	actuatorKey: ActuatorKey,
	isOn: boolean
): Promise<void> {
	try {
		const commandRef = ref(database, `actuators/${actuatorKey}/command`);
		// Simpan sebagai object dengan isOn property untuk consistency dengan ESP32
		await set(commandRef, {
			isOn,
			timestamp: Date.now()
		});
	} catch (error) {
		console.error(`Error updating actuator ${actuatorKey}:`, error);
		throw error;
	}
}

/**
 * Update system mode
 */
export async function updateSystemMode(
	mode: 'auto' | 'manual'
): Promise<void> {
	try {
		const modeRef = ref(database, 'system/mode');
		// Simpan sebagai string agar ESP32 bisa baca dengan getString()
		await set(modeRef, mode);
	} catch (error) {
		console.error('Error updating system mode:', error);
		throw error;
	}
}

/**
 * Subscribe to system mode
 */
export function subscribeSystemMode(
	onUpdate: (mode: 'auto' | 'manual') => void,
	onError: (error: Error) => void
) {
	try {
		const modeRef = ref(database, 'system/mode');
		const unsubscribe = onValue(
			modeRef,
			(snapshot) => {
				if (snapshot.exists()) {
					const data = snapshot.val();
					// Mode bisa berupa string langsung atau dalam object dengan mode property
					if (typeof data === 'string') {
						onUpdate(data as 'auto' | 'manual');
					} else if (data?.mode) {
						onUpdate(data.mode as 'auto' | 'manual');
					} else {
						onUpdate('auto');
					}
				}
			},
			(error: any) => onError(error as Error)
		);
		return unsubscribe;
	} catch (error) {
		onError(error as Error);
		return () => {};
	}
}
