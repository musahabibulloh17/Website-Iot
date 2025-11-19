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
		<div className="card" style={{ padding: 12, boxShadow: '0 10px 25px rgba(0, 0, 0, 0.5)' }}>
			<div style={{ fontSize: 11, color: 'var(--muted)', marginBottom: 8, fontWeight: 500 }}>{format(ts, 'dd MMM HH:mm')}</div>
			{payload.map((p: any) => (
				<div key={p.dataKey} style={{ display: 'flex', justifyContent: 'space-between', gap: 16, fontSize: 13 }}>
					<div><span className="dot" style={{ background: p.stroke, width: 6, height: 6 }} />{p.name}</div>
					<div style={{ fontWeight: 700, color: p.stroke }}>{p.value}</div>
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

	// Deteksi light mode
	const isLightMode = typeof document !== 'undefined' && document.body.classList.contains('light-mode');
	const gridColor = isLightMode ? 'rgba(0,0,0,0.06)' : 'rgba(255,255,255,0.08)';
	const axisColor = isLightMode ? 'rgba(51,65,85,0.3)' : 'rgba(148,163,184,0.3)';
	const refLineColor = isLightMode ? 'rgba(0,0,0,0.08)' : 'rgba(148,163,184,0.1)';

	return (
		<div className="card charts">
			<div style={{ marginBottom: 20 }}>
				<div className="title" style={{ fontSize: 20, margin: '0 0 12px 0' }}>{title}</div>
				<div className="legend">
					{series.map(s => (
						<div key={s.key}><span className="dot" style={{ background: SENSOR_META[s.key].color }} />{SENSOR_META[s.key].label}</div>
					))}
				</div>
			</div>
			<div style={{ width: '100%', height: 360 }}>
				<ResponsiveContainer>
					<LineChart data={data} margin={{ top: 8, right: 16, left: -20, bottom: 8 }}>
						<CartesianGrid stroke={gridColor} vertical={false} />
						<XAxis
							dataKey="ts"
							type="number"
							domain={[start, end]}
							tickFormatter={(ts) => format(ts, 'HH:mm')}
							ticks={hourlyLines}
							stroke={axisColor}
							style={{ fontSize: 12 }}
						/>
						<YAxis
							stroke={axisColor}
							style={{ fontSize: 12 }}
						/>
						<Tooltip content={<CustomTooltip />} />
						{/* Garis vertikal untuk setiap jam */}
						{hourlyLines.map((timestamp, i) => (
							<ReferenceLine
								key={`ref-${i}`}
								x={timestamp}
								stroke={refLineColor}
								strokeDasharray="4 4"
								opacity={0.6}
							/>
						))}
						{series.map(s => (
							<Line
								key={s.key}
								type="monotone"
								name={`${SENSOR_META[s.key].label} (${SENSOR_META[s.key].unit})`}
								dataKey={s.key}
								stroke={SENSOR_META[s.key].color}
								strokeWidth={2.5}
								isAnimationActive={false}
								dot={false}
								connectNulls
								opacity={0.85}
							/>
						))}
					</LineChart>
				</ResponsiveContainer>
			</div>
			<div className="subtitle" style={{ marginTop: 12, marginBottom: 0 }}>12 jam dari data pertama • Per menit</div>
		</div>
	);
}