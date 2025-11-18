import { format } from 'date-fns';
import {
	ResponsiveContainer,
	LineChart,
	Line,
	XAxis,
	YAxis,
	CartesianGrid,
	Tooltip,
	ReferenceLine
} from 'recharts';
import { SensorKey, SensorPoint } from '../types';
import { SENSOR_META } from '../data';

interface SeriesInput {
	key: SensorKey;
	points: SensorPoint[];
	hidden?: boolean;
}

interface Props {
	title: string;
	series: SeriesInput[];
}

function CustomTooltip({ active, payload, label }: any) {
	if (!active || !payload || !payload.length) return null;
	const ts = Number(label);
	return (
		<div className="card" style={{ padding: 10 }}>
			<div style={{ fontSize: 12, color: 'var(--muted)', marginBottom: 6 }}>{format(ts, 'dd MMM yyyy HH:mm')}</div>
			{payload.map((p: any) => (
				<div key={p.dataKey} className="row" style={{ justifyContent: 'space-between' }}>
					<div><span className="dot" style={{ background: p.stroke }} />{p.name}</div>
					<div style={{ fontWeight: 600 }}>{p.value}</div>
				</div>
			))}
		</div>
	);
}

export default function TimeSeriesChart({ title, series }: Props) {
	// Merge by timestamp for multi-line chart
	const allTs = series[0]?.points.map(p => p.timestamp) ?? [];
	const data = allTs.map(ts => {
		const row: any = { ts };
		series.forEach(s => {
			const p = s.points.find(pt => pt.timestamp === ts);
			if (p) row[s.key] = p.value;
		});
		return row;
	});

	// Hitung waktu mulai dari data pertama di database
	const minTimestamp = allTs.length > 0 ? Math.min(...allTs) : Date.now();
	const start = minTimestamp;
	const end = start + 12 * 60 * 60 * 1000; // 12 jam dari data pertama
	
	// Buat array referensi garis untuk setiap jam
	const hourlyLines = Array.from({ length: 13 }, (_, i) => start + i * 60 * 60 * 1000);

	return (
		<div className="card charts">
			<div className="row" style={{ marginBottom: 8 }}>
				<div className="title" style={{ fontSize: 18, margin: 0 }}>{title}</div>
				<div className="spacer" />
				<div className="legend">
					{series.map(s => (
						<div key={s.key}><span className="dot" style={{ background: SENSOR_META[s.key].color }} />{SENSOR_META[s.key].label}</div>
					))}
				</div>
			</div>
			<div style={{ width: '100%', height: 360 }}>
				<ResponsiveContainer>
					<LineChart data={data} margin={{ top: 8, right: 16, left: 0, bottom: 8 }}>
						<CartesianGrid stroke="rgba(0,0,0,0.1)" vertical={false} />
						<XAxis
							dataKey="ts"
							type="number"
							domain={[start, end]}
							tickFormatter={(ts) => format(ts, 'HH:mm')}
							ticks={hourlyLines}
							stroke="rgba(0,0,0,0.4)"
						/>
						<YAxis
							stroke="rgba(0,0,0,0.4)"
						/>
						<Tooltip content={<CustomTooltip />} />
						{/* Garis vertikal untuk setiap jam */}
						{hourlyLines.map((timestamp, i) => (
							<ReferenceLine
								key={`ref-${i}`}
								x={timestamp}
								stroke="rgba(0,0,0,0.15)"
								strokeDasharray="3 3"
								opacity={0.5}
							/>
						))}
						{series.map(s => (
							<Line
								key={s.key}
								type="monotone"
								name={`${SENSOR_META[s.key].label} (${SENSOR_META[s.key].unit})`}
								dataKey={s.key}
								stroke={SENSOR_META[s.key].color}
								strokeWidth={2}
								isAnimationActive={false}
								dot={false}
								connectNulls
							/>
						))}
					</LineChart>
				</ResponsiveContainer>
			</div>
			<div className="subtitle" style={{ marginTop: 6 }}>Per menit, grid 1 jam, dari data pertama database selama 12 jam</div>
		</div>
	);
}