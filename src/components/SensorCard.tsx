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
			<div style={{ fontSize: 12, color: 'var(--muted)', marginBottom: 12, fontWeight: 500 }}>
				{meta.label}
			</div>
			<div style={{ fontSize: 36, fontWeight: 700, lineHeight: 1, marginBottom: 8 }}>
				<span style={{ color: meta.color }}>{value}</span>
				<span style={{ fontSize: 14, color: 'var(--muted)', marginLeft: 6, fontWeight: 400 }}>{meta.unit}</span>
			</div>
			<div style={{ 
				fontSize: 11, 
				color: 'var(--muted)',
				display: 'flex',
				alignItems: 'center',
				gap: 6
			}}>
				<span style={{
					display: 'inline-block',
					width: 6,
					height: 6,
					borderRadius: '50%',
					background: meta.color,
					opacity: 0.8
				}}></span>
				Live
			</div>
		</div>
	);
}

