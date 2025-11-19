interface Props {
	theme: 'dark' | 'light';
	onToggle: () => void;
}

export default function ThemeToggle({ theme, onToggle }: Props) {
	return (
		<button
			className="theme-toggle"
			onClick={onToggle}
			aria-label={`Switch to ${theme === 'dark' ? 'light' : 'dark'} mode`}
			title={`${theme === 'dark' ? 'Light' : 'Dark'} Mode`}
		>
			{theme === 'dark' ? '☀️' : '🌙'}
		</button>
	);
}
