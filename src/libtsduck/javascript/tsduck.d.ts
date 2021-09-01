export class Report {
  constructor(arg1?: string, arg2?: string);
  clearLog(): void;
  getLog(arg?: string): string[];
}

export class AsyncReport extends Report {
  constructor();
  getLog(): string[];
}

export class CustomReport extends Report {
  constructor(arg1: string, arg2: string);
  getLog(arg?: string): string[];
  getLogMarker(): string;
}

export class TSProcessor {
  constructor(arg: Report);
  abort(): void;
  clearFields(): void;
  isStarted(): boolean;
  setInput(arg: string[]): void;
  setOutput(arg: string[]): void;
  setPlugins(arg: string[][]): void;
  start(): boolean;
  waitForTermination(): void;
}

export function getVersion(): string;
