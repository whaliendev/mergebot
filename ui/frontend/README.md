## conflict_vue

**!!! Attention: Please make sure to use `pnpm` to install dependencies and build
the project. This is because some issues in the key component `monaco-editor`,
such as NPE (Null Pointer Exception) and Array Index Out of Bound, have only
been fixed downstream (in the `patches` directory).**

### Prerequisites
- **Node.js** (version 18.x or higher recommended)
- **pnpm** (for managing dependencies and building the project)

You can install `pnpm` globally using npm:

```bash
npm install -g pnpm
```



### Development

#### 1. Set Up the Environment
**Install Dependencies**

```shell
pnpm install
```

> **Note:** Using `pnpm` ensures that the patched dependencies are correctly applied.

#### 2. Running the Development Server

Start the development server with hot-reloading enabled:

```bash
pnpm run serve
```

- The application will be accessible at `http://localhost:3000` by default.
- The development server supports hot module replacement (HMR) for a smooth development experience.

#### 3. Linting and Code Quality

Ensure your code adheres to the project's coding standards:

```bash
pnpm run lint
```

- This command will analyze your code and automatically fix linting issues where possible.
- It utilizes **ESLint** and **Prettier** for maintaining code quality and formatting.



### Build

#### Steps to Build for Production

1. **Compile and Minify the Project**
    ```bash
    pnpm run build
    ```
    - This will generate optimized and minified files in the `dist` directory, ready for deployment.

2. **Preview the Production Build**
    - To serve the production build locally, you can use a static server like `serve`:
      ```bash
      pnpm install -g serve
      serve -s dist
      ```
    - Navigate to the provided URL (usually `http://localhost:5000`) to view the production build.

#### Deployment

- **Static Hosting:** The contents of the `dist` directory can be deployed to any static hosting service such as **Nginx**, **Netlify**, **Vercel**, **GitHub Pages**, or your own server.




### Integration Guide for Additional Conflict Resolution Algorithms

Follow the steps below to integrate a new conflict resolution algorithm into the MergeBot platform:

1. **Add API Endpoints**
   - **Create API Module:** Add the corresponding API to the `src/api/` directory.
   - **Configure Base URL:** Set the base URL for the new API in `src/api/config.ts`.

2. **Update Triggers and Store**
   - **Add Trigger:** Incorporate the corresponding trigger in `src/views/DiffResolution.vue`.
   - **Store Data:** Store the retrieved data in the Vuex store located in `src/store/`.

3. **Handle Resolution Levels**
   
   - **Block Level Resolution:**
     1. **Modify Hover Contents:** Update the `extractHoverContents` function in `src/utils/ui.js` to add the necessary hover content.
     2. **Add Context Menu Actions:** Modify the `useAddContextMenuItems` function in `src/utils/ui.js` to include new context menu actions related to block-level resolution.
   
   - **Patch Level Resolution:**
     1. **Add Patch Interactions:** Update the `addPatchInteractions` function in `src/utils/ui.js` to handle patch-level interactions.
     2. **Update Context Menu for Patches:** Modify the `useAddContextMenuItemsToModified` function in `src/utils/ui.js` to add context menu actions for patch-level resolution.

4. **Finalize Integration**
   - **Verify Changes:** Ensure that all modifications are correctly implemented and that there are no conflicts or errors.
   - **Use the New Algorithm:** You can now utilize the newly integrated conflict resolution algorithm within the MergeBot frontend.

---

**Note:** After completing the integration, it is recommended to run the application and perform thorough testing to ensure that the new conflict resolution algorithm functions as expected without introducing any issues.




### Customize Configuration

Refer to the [Vue CLI Configuration Reference](https://cli.vuejs.org/config/) for detailed information on customizing your Vue project.

