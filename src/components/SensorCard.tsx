import { SENSOR_META } from '../data';
import { SensorKey } from '../types';

interface Props {
	sensor: SensorKey;
	value: number;
}

export default function SensorCard({ sensor, value }: Props) {
	const meta = SENSOR_META[sensor];
	return (
		<div className="card sensor-card">
			<div className="row">
				<div className="badge" style={{ borderColor: meta.color, color: meta.color }}>• {meta.label}</div>
				<div className="spacer" />
			</div>
			<div style={{ fontSize: 32, fontWeight: 700, lineHeight: 1.1, marginTop: 6 }}>
				<span style={{ color: meta.color }}>{value}</span>
				<span style={{ fontSize: 16, color: 'var(--muted)', marginLeft: 8 }}>{meta.unit}</span>
			</div>
		</div>
	);
}

