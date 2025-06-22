const RiDuckProvider = require('../src/pkjs/riduck/riduck');
global.XMLHttpRequest = require('xhr2');

jest.setTimeout(100000); // longer timeout for network calls

describe('RiDuckProvider Integration', () => {
  const provider = new RiDuckProvider();

  const username = process.env.RIDUCK_USERNAME;
  const password = process.env.RIDUCK_PASSWORD;

  if (!username || !password) {
    throw new Error('⚠️ Missing RIDUCK_USERNAME or RIDUCK_PASSWORD in environment variables.');
  }

  test('should log in and fetch dashboard advice', (done) => {
    provider.login(username, password, (token) => {
      expect(typeof token).toBe('string');
      expect(token.length).toBeGreaterThan(10); // rudimentary check

      provider.fetchAdvice(token, (adviceNumber) => {
        expect(typeof adviceNumber).toBe('number');
        expect(adviceNumber).toBeGreaterThanOrEqual(0);
        expect(adviceNumber).toBeLessThanOrEqual(7000);
        done();
      });
    });
  });
});
