const gulp = require('gulp');
const htmlmin = require('gulp-htmlmin');
const inlineSource = require('gulp-inline-source');
const replace = require('gulp-replace');
const del = require('del');
const { rollup } = require('rollup'); // Use rollup API
const resolve = require('@rollup/plugin-node-resolve').default;
const terser = require('@rollup/plugin-terser'); // Use rollup's terser plugin

const paths = {
    html: 'src/index.html',
    js_entry: 'src/script.js',
    css: 'src/style.css',
    dest: 'build',
    mainDest: '../main'
};

// Clean the destination directory
const clean = () => del([paths.dest], { force: true });

// Bundle and minify JavaScript using Rollup
const scripts = async () => {
    const bundle = await rollup({
        input: paths.js_entry,
        plugins: [
            resolve(),
            terser() // Minify JS
        ]
    });

    return bundle.write({
        file: `${paths.dest}/script.js`,
        format: 'iife', // Immediately Invoked Function Expression for browser
        sourcemap: false
    });
};

// Copy assets to build directory for inlining
const copyAssets = () => {
    return gulp.src([paths.html, paths.css])
        .pipe(gulp.dest(paths.dest));
};

// Build task: inline CSS/JS and minify everything into a single HTML file
const buildHtml = () => {
    return gulp.src(`${paths.dest}/index.html`)
        .pipe(replace('<script src="script.js" type="module" inline></script>', '<script src="script.js" inline></script>'))
        .pipe(inlineSource({
            rootpath: paths.dest,
            compress: true
        }))
        .pipe(htmlmin({
            collapseWhitespace: true,
            removeComments: true,
            minifyCSS: true,
            minifyJS: false // JS is already minified by rollup-plugin-terser
        }))
        .pipe(gulp.dest(paths.dest))
        .pipe(gulp.dest(paths.mainDest));
};

// Clean intermediate files from build directory
const cleanBuild = () => del([`${paths.dest}/script.js`, `${paths.dest}/style.css`], { force: true });

const watch = () => {
    gulp.watch('src/**/*', gulp.series(clean, gulp.parallel(scripts, copyAssets), buildHtml, cleanBuild));
};

const build = gulp.series(clean, gulp.parallel(scripts, copyAssets), buildHtml, cleanBuild);

exports.clean = clean;
exports.scripts = scripts;
exports.build = build;
exports.watch = watch;
exports.default = build;
