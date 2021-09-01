//---------------------------------------------------------------------------
//
// TSDuck sample Javascript application running a chain of plugins.
//
//----------------------------------------------------------------------------

const tsduck = require('tsduck');

console.log(`TSDuck version: ${tsduck.getVersion()}`);

/*
 * Creates an asynchronous custom report to log multi-threaded messages.
 */
const rep = new tsduck.AsyncReport();

/*
 * Create a TS processor using the report.
 */
const tsp = new tsduck.TSProcessor(rep);

/*
 * Set the plugin chain.
 */
tsp.setInput([
  'craft',
  '--count',
  '1000',
  '--pid',
  '100',
  '--payload-pattern',
  '0123',
]);
tsp.setPlugins([['until', '--packet', '100'], ['count']]);
tsp.setOutput(['drop']);

/*
 * Run the TS processing and log the result.
 */
(async () => {
  await tsp.start();
  console.log(rep.getLog());
})();
