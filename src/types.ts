export type SensorKey = 'light' | 'airHumidity' | 'soilMoisture' | 'airTemp';

export interface SensorPoint {
	timestamp: number; // ms epoch
	value: number;
}

export type SensorSeries = Record<SensorKey, SensorPoint[]>;

export type ActuatorKey = 'lamp' | 'fan' | 'pump';

export interface ActuatorState {
	key: ActuatorKey;
	name: string;
	isOn: boolean;
}

export interface ControlMode {
	mode: 'auto' | 'manual';
}

export interface DashboardState {
	series: SensorSeries;
	current: Record<SensorKey, number>;
	mode: ControlMode['mode'];
	actuators: Record<ActuatorKey, ActuatorState>;
}



